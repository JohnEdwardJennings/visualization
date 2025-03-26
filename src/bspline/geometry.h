#include <cstddef>
#include <vector>
#include <array>
#include <span>

/*
 * A data structure for holding the parameters for a B-Spline.
 * Includes degrees, knot vectors, and control points.
 * 
 * Parametric points at which to evaluate the B-Spline 
 * are passed as arguments to the evaluate() functions.
 */
template <size_t n_kdims, size_t n_cdims, size_t n_threads = 1>
class BSplineGeometry {
private:
	/*
	 * The type of scalars (coordinates of knots and control points).
	 * Here, we use double-precision floating point numbers.
	 */
	using scalar_t = double;
	
	/*
	 * The datatype of parametric points (including knots).
	 * If k = n_kdims, then these points lie in the space R^k.
	 */
	using knot_t = std::array<scalar_t, n_kdims>;
	
	/*
	 * The datatype of physical points, or of control points.
	 * If c = n_cdims, then these points lie in the space R^c.
	 * 
	 * The type of the spline function is 
	 * "knot_t -> ctrl_t", or "R^k -> R^c". 	
	 */
	using ctrl_t = std::array<scalar_t, n_cdims>; 

	/*
	 * The information associated with each parametric dimension.
	 * This includes:
	 * 	* the degree in that dimension
	 * 	* the vector of knot coordinates along that axis
	 * 	* the number of knots, excluding any padding knots
	 */
	struct param {
		size_t degree;
		std::vector<scalar_t> knot_vector;
		size_t n_knots;	
	};
	std::array<param, n_kdims> params;
	
	/*
	 * The array of control points.
	 * 
	 * For convenience, simplicity of template code, and possibly
	 * performance reasons, this is a single flattened array, not
	 * a depth-k nested array/radix tree.
	 * 
	 * The control points are ordered lexicographically:
	 * each knot can be described by a k-tuple of indices 
	 * J = (j_0, j_1, ..., j_{k-1}) such that each j_s is a
	 * valid index in the knot vector for dimension s.
	 *
	 * Letting l_s be the number of elements in the knot vector 
	 * in dimension s, excluding padding, we have 0 <= j_s < r_s
	 * for all s, and the index of the control point for J is
	 * I_k, where
	 * I_0 = 0,
	 * I_{s+1} = j_s + l_s * I_s for 0 <= s < k. 
	 */
	std::vector<ctrl_t> control_points;

	/*
	 * Scratch space for recursive calculations of B-Spline
	 * basis functions.
	 * 
	 * This approach avoids allocating and deallocating memory 
	 * every time the BSpline is evaluated.
	 *
	 * However, since the space is not owned by the evaluate()
	 * function, multiple concurrent evaluations of the same
	 * BSplineGeometry will lead to race conditions and
	 * corrupted values for the spline function, unless more
	 * than one scratch space is used.
	 *
	 * A multithreaded implementation (n_threads > 1) needs to 
	 * include as many scratch spaces as threads and assign a
	 * separate scratch space to each thread. 
	 */
	struct scratch_space {
		size_t offset;
		scalar_t* store;

		scratch_space() {}

		scratch_space(std::array<size_t, n_kdims> degrees) {
			size_t max_degree = 0;
			for (size_t s = 0; s < n_kdims; s++) {
				if (degrees[s] > max_degree) {
					max_degree = degrees[s];
				}
			}
			offset = max_degree + 1;
			store = new scalar_t[offset * (n_kdims + 1)];
		}
		~scratch_space() {
			delete[] store;
		}
		scalar_t* row(size_t index) const {
			return store + index * offset;
		}
	};
	std::array<scratch_space, n_threads> scratch;

public:
	
	/* Constructor */
	BSplineGeometry(std::array<size_t, n_kdims> degrees, 
				std::array<std::vector<scalar_t>, n_kdims> knot_vectors, 
				std::vector<ctrl_t> control_points)
	{
		for (size_t s = 0; s < n_kdims; s++) {
			params[s] = param{degrees[s], knot_vectors[s], knot_vectors[s].size()};
		}
		control_points = control_points;
		for (size_t z = 0; z < n_threads; z++) {
			scratch[z] = scratch_space(degrees);
		}
		/* Check that the BSplineGeometry state is valid */
	}

	/*
	 * Evaluate the spline at a parametric point x.
	 * Store the result in y.
	 *
	 * tid is an optional argument specifying a unique
	 * identification number (with 0 <= tid < n_threads)
	 * for the thread doing this evaluation. In
	 * single threaded implementations (n_threads = 1)
	 * the default value of 0 should be used.
	 */
	void evaluate(knot_t const& x, ctrl_t& y, size_t tid = 0) const 
	{
		// check that x is in bounds in every dimension
		for (size_t s = 0; s < n_kdims; s++) {
			if (x[s] < params[s].knot_vector.front() 
				|| x[s] > params[s].knot_vector.back()) {
				// TODO report error
			}
		}
		
		scratch_space const& space = scratch[tid];
		std::array<size_t, n_kdims> first, last;
		for (size_t s = 0; s < n_kdims; s++) {
			// convenience variables
			scalar_t u = x[s];
			size_t p = params[s].degree;
		    	std::vector<scalar_t> const& t = params[s].knot_vector;
			size_t l = t.size();

			/*
			 * Find the knot span in which u lies.
			 * This is an interval [t_j,t_{j+1}) such that
			 * t_j <= u < t_{j+1}.
			 *
			 * We avoid the cases j < p or j >= l - p - 1,
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
			size_t j, lo = p, hi = l - p - 1;
			while (true) {
				j = (lo + hi) / 2;
				if (t[j] <= u) {
					if (j == l - p - 2) break;
					if (t[j + 1] <= u) lo = j + 1;
					else break;
				}
				else hi = j;
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
			scalar_t *B = space.row(s), *C = space.row(n_kdims);
			if (p % 2 == 1) {
				std::swap(B,C);
			}
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
		std::array<size_t, n_kdims> pos = first;
		
		// stack of 'prefix indices' for the control array
		// the top (last element) is the actual index
		std::array<size_t, n_kdims + 1> istack{0};
		
		// stack of prefix products for convenience
		// the top (last element) is the actual weight
		std::array<scalar_t, n_kdims + 1> bstack{1};
		
		// level variable for backtracking
		size_t s = 0;
		while (true) {
			/* 'Push' stage.
			 * Compute and store indices and weights recursively. 
			 */
			do {
				istack[s + 1] = pos[s] - params[s].degree + params[s].n_knots * istack[s];
				bstack[s + 1] = space.row(s)[pos[s] - first[s]] * bstack[s];
				s++;	
			} while (s < n_kdims);

			/* Main computation */
			size_t I = istack[s + 1];
			scalar_t B = bstack[s + 1];
			ctrl_t const& ctrl_pt = control_points[I];
			for (size_t r = 0; r < n_cdims; r++) {
				y[r] += B * ctrl_pt[r];
			}

			/* 'Pop' stage.
			 * Update position, check exit conditions for each level
			 * and for the entire algorithm, and reset  
			 */
			while (true) {
				pos[--s]++;
				if (pos[s] <= last[s]) break;
				if (s == 0) return;
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
	std::vector<ctrl_t> evaluate(std::vector<knot_t> const& x) const
	{
		std::vector<ctrl_t> y{x.size()};
		for (auto xi = x.begin(), yi = y.begin(); xi != x.end(); ++xi, ++yi) {
			evaluate(*xi, *yi);
		}
		return y;
	}
};

/* Explicit instantiation of BSplineGeometries for dimensions 1,2,3. */
typedef BSplineGeometry<1,1> BSplineGeometry_1D_1D;
typedef BSplineGeometry<1,2> BSplineGeometry_1D_2D; 
typedef BSplineGeometry<1,3> BSplineGeometry_1D_3D;
typedef BSplineGeometry<2,1> BSplineGeometry_2D_1D;
typedef BSplineGeometry<2,2> BSplineGeometry_2D_2D;
typedef BSplineGeometry<2,3> BSplineGeometry_2D_3D;
typedef BSplineGeometry<3,1> BSplineGeometry_3D_1D;
typedef BSplineGeometry<3,2> BSplineGeometry_3D_2D;
typedef BSplineGeometry<3,3> BSplineGeometry_3D_3D;
