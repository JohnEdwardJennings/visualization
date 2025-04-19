#include "geometry.h"
#include <cstddef>
#include <vector>
#include <iostream>
using namespace std;

void error(char const * msg)
{
	printf("Error: %s\n", msg);
	exit(1);		
}

/* Constructor */
BSplineGeometry::BSplineGeometry(
			size_t n_kdims,
		       	size_t n_cdims, 
			std::vector<size_t> const& degrees, 
			std::vector<std::vector<scalar_t>> const& knot_vectors, 
			std::vector<ctrl_t> const& control_points,
			size_t n_threads)
	: n_kdims(n_kdims), n_cdims(n_cdims), n_threads(n_threads), params(n_kdims), scratch(n_threads) 
{
	/* Check that the BSplineGeometry state is valid. */

	/* n_kdims, n_cdims, and n_threads must be positive */
	if (n_kdims == 0) {
		error("cannot create BSplineGeometry with zero-dimensional parametric points");
	}
	if (n_cdims == 0) {
		error("cannot create BSplineGeometry with zero-dimensional control points");
	}
	if (n_threads == 0) {
		error("cannot create BSplineGeometry with zero threads");
	}

	/* The dimensions of the rest of the input must match the size parameters. */
	if (degrees.size() != n_kdims) {
		error("incorrect number of degrees provided");
	}
	if (knot_vectors.size() != n_kdims) {
		error("incorrect number of knot vectors provided");
	}
	for (ctrl_t point : control_points) {
		if (point.size() != n_cdims) {
			error("control point has incorrect dimension");
		}
	}

	/* All knot vectors should be in nonstrictly increasing order. */
	for (size_t s = 0; s < n_kdims; s++) {
		std::vector<scalar_t> const& kv = knot_vectors[s];		
		for (size_t i = 0; i + 1 < kv.size(); i++) {	
			if (kv[i + 1] < kv[i]) {
				error("knot vector out of order");
			}
		}	
	}

	/* In each dimension, at least one knot span should be nonempty. */
	for (size_t s = 0; s < n_kdims; s++) {
		std::vector<scalar_t> const& kv = knot_vectors[s];	
		if (kv.size() == 0) {
			error("empty knot vector");
		}
		if (kv.front() == kv.back()) {
			error("no nonempty knot spans");
		}
	}

	/*
	 * The number of control points should equal
	 * exactly the product across all dimensions of 
	 * the degree plus the number of (legitimate, 
	 * not padding) knots, minus one.
	 */
	size_t ctrl_sz = 1;
	for (size_t s = 0; s < n_kdims; s++) {
		ctrl_sz *= (knot_vectors[s].size() + degrees[s] - 1);
	}
	if (control_points.size() != ctrl_sz) {
		error("incorrect number of control points");
	}
	
	/* Construct the BSpline object */

	for (size_t s = 0; s < n_kdims; s++) {
		params[s].degree = degrees[s];
		params[s].n_ctrl = knot_vectors[s].size() + degrees[s] - 1;
	}
	this->control_points = control_points;

	for (size_t z = 0; z < n_threads; z++) {
		size_t max_degree = 0;
		for (size_t s = 0; s < n_kdims; s++) {
			if (degrees[s] > max_degree) {
				max_degree = degrees[s];
			}
		}
		scratch[z].offset = max_degree + 1;
		scratch[z].store = std::vector<scalar_t>(scratch[z].offset * (n_kdims + 1));
	}
		
	/* 
	 * Compute the index of the highest nonempty knot span.
	 * 
	 * This is used to handle the edge case of evaluating
	 * the spline at a parametric point whose coordinate
	 * lies exactly on the upper edge. Placing the point in an
	 * empty knot span (which might happen if the two highest
	 * non-padding knots are identical) would cause division by 
	 * zero, so when finding the knot span for such a point we 
	 * cap the index at the highest nonempty knot span.
	 */
	for (size_t s = 0; s < n_kdims; s++) {
		std::vector<scalar_t> const& kv = knot_vectors[s];
		if (kv.size() == 0) {
			error("empty knot vector");
		}
		size_t max_span = kv.size() - 2;
		scalar_t last_knot = kv.back();
		while (kv[max_span] == last_knot) {
			max_span--;
		}
		params[s].span_cap = max_span + degrees[s];
	}

	/*
	 * Insert padding knots at the beginning and end of the spline. 
	 * 
	 * This assumes the spline is clamped and may not be the right 
	 * approach for some types of spline (e.g. periodic).
	 */
	for (size_t s = 0; s < n_kdims; s++) {
		std::vector<scalar_t> const& kv = knot_vectors[s];
		size_t d = degrees[s], len = kv.size();
		std::vector<scalar_t> padded(d + len + d);
		size_t i = 0;
		for (; i < d; i++) {
			padded[i] = kv[0];
		}
		for (; i < d + len; i++) {
			padded[i] = kv[i - d];
		}
		for (; i < d + len + d; i++) {
			padded[i] = kv[len - 1];
		}
		params[s].knot_vector = padded;
	}
}

ctrl_t BSplineGeometry::evaluate(knot_t const& x, size_t tid) 
{
	// check that x has the correct number of coordinates
	if (x.size() != n_kdims) {
		error("dimensions of evaluation point do not match B-spline geometry");
	}

	// check that x is in bounds in every dimension
	for (size_t s = 0; s < n_kdims; s++) {
		if (x[s] < params[s].knot_vector.front() 
			|| x[s] > params[s].knot_vector.back()) {
			error("evaluating at out-of-bounds point");
		}
	}
	
	scratch_space& space = scratch[tid];
	std::vector<size_t> first(n_kdims), last(n_kdims);
	for (size_t s = 0; s < n_kdims; s++) {
		// convenience variables
		scalar_t u = x[s];
		size_t p = params[s].degree;
		size_t l = params[s].span_cap;
		std::vector<scalar_t> const& t = params[s].knot_vector;
		
		/*
		 * Find the knot span in which u lies.
		 * This is an interval [t_j,t_{j+1}) such that
		 * t_j <= u < t_{j+1}.
		 *
		 * We avoid the cases j < p or j >= l,
		 * because these knot spans are empty
		 * and used only for padding. In the case
		 * where u equals the maximum knot value,
		 * this requires special treatment, since
		 * technically u < t_{j+1} is not possible.
		 * We settle for allowing u = t_{j+1} then,
		 * and we require t_j < u instead.
		 *
		 * We use the binary search algorithm,
		 * since the knot vectors are sorted.
		 */
		size_t j, lo = p, hi = l;
		if (u == t.back()) {
			j = l;	
		}
		else {
			while (true) {
				j = (lo + hi) / 2;
				if (t[j] <= u) {
					if (t[j + 1] <= u) lo = j + 1;
					else break;
				}
				else hi = j - 1;
			}
		}
		first[s] = j - p;
		last[s] = j;

		/*
		 * Precompute the B-Spline basis functions.
		 *
		 * We use a tabulation approach to reduce
		 * the total number of computations from
		 * O(d^3) to O(d^2).
		 *
		 * Instead of the top-down Cox-de Boor
		 * recursion formula, we implement
		 * a bottom-up approach similar to
		 * de Boor's algorithm but without coupling
		 * linear combination of the control points
		 * with basis function computation. This
		 * allows us to store and use the computed
		 * basis functions as weights for multiple 
		 * control points in dimension k >= 2.
		 */
		std::vector<scalar_t>::iterator B = space.row(s), C = space.row(n_kdims);
		if (p % 2 == 1) {
			std::swap(B, C);
		}
		std::fill(B, B + p, 0);
		std::fill(C, C + p + 1, 0);
		B[p] = 1;
		for (size_t q = 1; q <= p; q++) {
			size_t idx = p - q, i = j - q;
			C[idx] = ((t[i + q + 1] - u) / (t[i + q + 1] - t[i + 1])) * B[idx + 1];
			idx++, i++;
			for (; idx < p; idx++, i++) {
				C[idx] = ((u - t[i]) / (t[i + q] - t[i])) * B[idx]
					+ ((t[i + q + 1] - u) / (t[i + q + 1] - t[i + 1])) * B[idx + 1]; 
			}
			C[idx] = ((u - t[i]) / (t[i + q] - t[i])) * B[idx];
			std::swap(B, C);
		}
	}

	/*
	 * Compute the linear combinations of control points 
	 * weighted by a tensor product of B-Spline basis functions.
	 *
	 * Since the number of dimensions and thus the number of
	 * one-dimensional loops is not fixed for this template,
	 * we use a form of iterative backtracking to iterate over
	 * the relevant control points in any number of dimensions. 
	 */
	
	// position in knot grid
	std::vector<size_t> pos = first;

	// stack of 'prefix indices' for the control array
	// the top (last element) is the actual index
	std::vector<size_t> istack(n_kdims + 1);
	istack[0] = 0;

	// stack of prefix products for convenience
	// the top (last element) is the actual weight
	std::vector<scalar_t> bstack(n_kdims + 1);
	bstack[0] = 1;

	// output variable, initialized to zero in all coordinates
	ctrl_t y(n_cdims);

	// level variable for backtracking
	size_t s = 0;
	while (true) {
		/*
		 * 'Push' stage.
		 * Compute and store indices and weights recursively. 
		 */
		do {
			istack[s + 1] = pos[s] + params[s].n_ctrl * istack[s];
			// FIXME gdb says the LHS is zero when the RHS is not.
			// This issue only seems to arise for degree >= 2, weirdly.
			// A possible explanation is the swapping of iterators?
			bstack[s + 1] = space.row(s)[pos[s] - first[s]] * bstack[s];
			s++;	
		} while (s < n_kdims);

		/* Main computation */
		size_t I = istack[s];
		scalar_t B = bstack[s];
		ctrl_t const& ctrl_pt = control_points[I];
		for (size_t r = 0; r < n_cdims; r++) {
			y[r] += B * ctrl_pt[r];
		}

		/*
		 * 'Pop' stage.
		 * Update position, check exit conditions for each level
		 * and for the entire algorithm, and reset  
		 */
		while (true) {
			pos[--s]++;
			if (pos[s] <= last[s]) break;
			if (s == 0) return y;
			pos[s] = first[s];
		}
	}
}

	/*
	 * Map operation.
	 * Compute the spline at a collection of parametric points.
	 *
	 * The input x is mapped to a vector y such that for all
	 * valid indices i of x, y[i] is the spline evaluated at x[i].
	 */
std::vector<ctrl_t> BSplineGeometry::evaluate(std::vector<knot_t> const& x)
{
	std::vector<ctrl_t> y{x.size()};
	auto xi = x.begin();
	auto yi = y.begin();
	for (; xi != x.end(); ++xi, ++yi) {
		*yi = evaluate(*xi);
	}
	return y;
}


// Tests
int main() 
{
	// hardcoded test cases

	{
		// n_kdims = 1, n_cdims = 1, degree = 0
		std::vector<size_t> degrees{0};
		knot_t _knots{0, 0.5, 1, 1, 1, 1, 1, 1};
		std::vector<knot_t> knots{{0, 0.5, 1, 1, 1, 1, 1, 1}};
		std::vector<ctrl_t> control_points{{0}, {0.5}, {-1}, {-2}, {-3}, {-4}, {-5}};
		BSplineGeometry spline = BSplineGeometry(1, 1, degrees, knots, control_points);
		
		std::vector<knot_t> x{{0}, {0.25}, {0.5}, {0.75}, {1}};
		
		std::vector<ctrl_t> y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			for (auto j = i->begin(); j < i->end(); j++) {
				cout << *j << " ";
			}
			cout << "\n";		
		}
		cout << "\n";
	}	
	
	{
		// n_kdims = 1, n_cdims = 2, degree = 0
		std::vector<size_t> degrees{0};
		std::vector<knot_t> knots{{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1}};
		std::vector<ctrl_t> control_points{{0, 1}, {0.125, 2}, {0.25, 4}, {0.375, 8}, {0.5, 16}, {0.625, 32}, {0.75, 64}, {0.875, 128}};
		BSplineGeometry spline = BSplineGeometry(1, 2, degrees, knots, control_points);
		
		std::vector<knot_t> x{{0}, {0.25}, {0.5}, {0.75}, {1}};
		
		std::vector<ctrl_t> y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			for (auto j = i->begin(); j < i->end(); j++) {
				cout << *j << " ";
			}
			cout << "\n";		
		}
		cout << "\n";
	}

	{
		// n_kdims = 2, n_cdims = 2, degree = 0
		std::vector<size_t> degrees{0, 0};
		std::vector<std::vector<double>> knots{{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1}, {0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1}};
		std::vector<std::vector<double>> control_points {	
			{0, 1}, {0.125, 2}, {0.25, 4}, {0.375, 8}, {0.5, 16}, {0.625, 32}, {0.75, 64}, {0.875, 128},
			{0, 1.1}, {0.125, 2.1}, {0.25, 4.1}, {0.375, 8.1}, {0.5, 16.1}, {0.625, 32.1}, {0.75, 64.1}, {0.875, 128.1},
			{0, 1.2}, {0.125, 2.2}, {0.25, 4.2}, {0.375, 8.2}, {0.5, 16.2}, {0.625, 32.2}, {0.75, 64.2}, {0.875, 128.2},
			{0, 1.3}, {0.125, 2.3}, {0.25, 4.3}, {0.375, 8.3}, {0.5, 16.3}, {0.625, 32.3}, {0.75, 64.3}, {0.875, 128.3},
			{0, 1.4}, {0.125, 2.4}, {0.25, 4.4}, {0.375, 8.4}, {0.5, 16.4}, {0.625, 32.4}, {0.75, 64.4}, {0.875, 128.4},
			{0, 1.5}, {0.125, 2.5}, {0.25, 4.5}, {0.375, 8.5}, {0.5, 16.5}, {0.625, 32.5}, {0.75, 64.5}, {0.875, 128.5},
			{0, 1.6}, {0.125, 2.6}, {0.25, 4.6}, {0.375, 8.6}, {0.5, 16.6}, {0.625, 32.6}, {0.75, 64.6}, {0.875, 128.6},
			{0, 1.7}, {0.125, 2.7}, {0.25, 4.7}, {0.375, 8.7}, {0.5, 16.7}, {0.625, 32.7}, {0.75, 64.7}, {0.875, 128.7}
		};
		auto spline = BSplineGeometry(2, 2, degrees, knots, control_points);
		
		std::vector<std::vector<double>> x{{0, 0}, {0.25, 0.25}, {0.5, 0.5}, {0.75, 0.75}, {1, 1}, 
			{0.125, 0.999}, {0.376, 0.75}, {0.5, 0.626}, {0.999, 0.375}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			for (auto j = i->begin(); j < i->end(); j++) {
				cout << *j << " ";
			}
			cout << "\n";		
		}
		cout << "\n";
	}

	{
		// n_kdims = 3, n_cdims = 3, degree = 0
		std::vector<size_t> degrees{0, 0, 0};
		std::vector<std::vector<double>> knots{{0, 0.33, 0.67, 1}, {0, 0.33, 0.67, 1}, {0, 0.33, 0.67, 1}};
		std::vector<std::vector<double>> control_points {	
			{100, 10, 1}, {100, 10, 2}, {100, 10, 3}, {100, 20, 1}, {100, 20, 2}, {100, 20, 3}, {100, 30, 1}, {100, 30, 2}, {100, 30, 3},
			{200, 10, 1}, {200, 10, 2}, {200, 10, 3}, {200, 20, 1}, {200, 20, 2}, {200, 20, 3}, {200, 30, 1}, {200, 30, 2}, {200, 30, 3},
			{300, 10, 1}, {300, 10, 2}, {300, 10, 3}, {300, 20, 1}, {300, 20, 2}, {300, 20, 3}, {300, 30, 1}, {300, 30, 2}, {300, 30, 3}
		};
		auto spline = BSplineGeometry(3, 3, degrees, knots, control_points);
		
		std::vector<std::vector<double>> x{{1, 0.5, 0}, {0.5, 0, 1}, {0, 1, 0.5}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			for (auto j = i->begin(); j < i->end(); j++) {
				cout << *j << " ";
			}
			cout << "\n";		
		}
		cout << "\n";
	}
	
	{
		// n_kdims = 1, n_cdims = 1, degree = 1
		std::vector<size_t> degrees{1};
		std::vector<std::vector<double>> knots{{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1}};
		std::vector<std::vector<double>> control_points{{1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}};
		auto spline = BSplineGeometry(1, 1, degrees, knots, control_points);
		
		std::vector<std::vector<double>> x{{0}, {0.125}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			for (auto j = i->begin(); j < i->end(); j++) {
				cout << *j << " ";
			}
			cout << "\n";		
		}
		cout << "\n";
	}
	
	{
		// n_kdims = 1, n_cdims = 1, degree = 2
		std::vector<size_t> degrees{2};
		std::vector<std::vector<double>> knots{{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1}};
		std::vector<std::vector<double>> control_points{{1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}};
		auto spline = BSplineGeometry(1, 1, degrees, knots, control_points);
		
		std::vector<std::vector<double>> x{{0}, {0.125}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
			
		for (auto i = y.begin(); i != y.end(); i++) {
			for (auto j = i->begin(); j < i->end(); j++) {
				cout << *j << " ";
			}
			cout << "\n";		
		}
		cout << "\n";
	}
	
	{
		// n_kdims = 1, n_cdims = 1, degree = 10
		std::vector<size_t> degrees{10};
		std::vector<std::vector<double>> knots{{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1}};
		std::vector<std::vector<double>> control_points{{1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}};
		auto spline = BSplineGeometry(1, 1, degrees, knots, control_points);
		
		std::vector<std::vector<double>> x{{0}, {0.125}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			for (auto j = i->begin(); j < i->end(); j++) {
				cout << *j << " ";
			}
			cout << "\n";		
		}
		cout << "\n";
	}
}
