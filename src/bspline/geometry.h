#include <cstddef>
#include <vector>

/*
 * The type of scalars (coordinates of knots and control points).
 * Here, we use double-precision floating point numbers.
 */
typedef double scalar_t;

/*
 * The datatype of parametric points (including knots).
 * Any parameter marked knot_t should contain exactly n_kdims scalars.
 * If k = n_kdims, then these points lie in the space R^k.
 */
typedef std::vector<scalar_t> knot_t;

/*
 * The datatype of physical points, or of control points.
 * Any parameter marked ctrl_t should contain exactly n_cdims scalars.
 * If c = n_cdims, then these points lie in the space R^c.
 * 
 * The type of the spline function is 
 * "knot_t -> ctrl_t", or "R^k -> R^c". 	
 */
typedef std::vector<scalar_t> ctrl_t;


/*
 * A data structure for holding the parameters for a B-Spline.
 * Includes degrees, knot vectors, and control points.
 * 
 * Parametric points at which to evaluate the B-Spline 
 * are passed as arguments to the evaluate() functions.
 */
class BSplineGeometry {
private:
	/* The number of parametric points */
	size_t n_kdims;
	/* The number of control points */
	size_t n_cdims;

	/*
	 * The information associated with each parametric dimension.
	 * This includes:
	 * 	* the degree in that dimension
	 * 	* the index of the highest nonempty knot span
	 * 	* the number of control point layers in that dimension
	 * 		(the degree plus the number of legitimate
	 * 		 knots, not including padding)
	 * 	* the vector of knot coordinates along that axis
	 */
	struct param {
		size_t degree;
		size_t span_cap;
		size_t n_ctrl;
		std::vector<scalar_t> knot_vector;		
	};
	std::vector<param> params;
	
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
	size_t n_threads;
	struct scratch_space {
		size_t offset;
		std::vector<scalar_t> store;

		std::vector<scalar_t>::iterator row(size_t index) 
		{
			return store.begin() + index * offset;
		}
	};
	std::vector<scratch_space> scratch;

public:
	
	/* Constructor */
	BSplineGeometry(
			size_t n_kdims,
		       	size_t n_cdims, 
			std::vector<size_t> const& degrees, 
			std::vector<std::vector<scalar_t>> const& knot_vectors, 
			std::vector<ctrl_t> const& control_points,
			size_t n_threads = 1);
	
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
	ctrl_t evaluate(knot_t const& x, size_t tid = 0); 

	/*
	 * Map operation.
	 * Compute the spline at a collection of parametric points.
	 *
	 * The input x is mapped to a vector y such that for all
	 * valid indices i of x, y[i] is the spline evaluated at x[i].
	 */
	std::vector<ctrl_t> evaluate(std::vector<knot_t> const& x);
};
