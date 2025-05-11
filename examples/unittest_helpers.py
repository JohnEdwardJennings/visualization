import numpy as onp
from scipy.interpolate import BSpline as scipyBSpline
import matplotlib.pyplot as plt
from typing import Union, Literal

from cardiax.generate_mesh import box_mesh_bspline
from cardiax.iga import BSpline


def create_scipy_bspline(fe):
        """
        """
        # basis func in u
        N_u = create_bspline_basis_fun(fe.knots[0], fe.degrees[0])
        
        # basis func in v
        N_v = create_bspline_basis_fun(fe.knots[1], fe.degrees[1])
        
        # basis func in w
        N_w = create_bspline_basis_fun(fe.knots[2], fe.degrees[2])
        
        return N_u, N_v, N_w

def eval_scipy_bspline_ele(N_u, N_v, N_w, grid_points, control_points, dim=1):

    # get out of range error when evaluating at knots (other than 0)... not quite sure why this is.
    eval_field = onp.zeros((len(grid_points),dim))
    
    # might need to come up with a way to identify         
    for l, point in enumerate(grid_points):
        count = 0    
        for i in range(len(N_u)):
            for j in range(len(N_v)):
                for k in range(len(N_w)):
                    # check if all basis functions are supported
                    # leads to a lot of unnecessary computation, but this is just for testing purposes.
                    N_u_i = N_u[i](point[0], False)
                    N_v_j = N_v[j](point[1], False)
                    N_w_k = N_w[k](point[2], False)
                    product = N_u_i * N_v_j * N_w_k
                    if onp.isnan(product):
                        continue
                    else:
                        #eval_field[l] += control_points[i,j,k] * onp.nan_to_num(N_u[i](point[0], False)) * onp.nan_to_num(N_v[j](point[1], False)) * onp.nan_to_num(N_w[k](point[2], False))
                        eval_field[l] += control_points[count] * product
                        count += 1
    return eval_field

# currently, I pass ALL of the basis functions...should identify which ones are supported.
# 1. map [i,j,k] -> num cp
# 2. support[0] -> num cp that is supported on the cell -> get [i,j,k] tuple 
# 3. pass the basis functions required to compute tensor product at [i,j,k] tuple
# note: showing this is good, but there is likely an indexing by 1 error in the for loop... will check later though.
#       (not good for dev, but good for communication/slides purposes)
def scipy_extraction_operator(N_u, N_v, N_w, lagrange_points, cp_ordering=None, dim=1, SAVE_C_S=False):
    """ generates lagrange extraction operator

    lagrange points must be ordered in the same manner which they are ordered within the extraction
    code (currently/as of 2/28/25), control points get reshuffled appropriately.

    # note: there is a chance this is just an ordering debacle...this test will help determine this!

    Parameters
    ----------
    N_u : _type_
        _description_
    N_v : _type_
        _description_
    N_w : _type_
        _description_
    lagrange_points : _type_
        _description_
    cp_ordering : _type_, optional
        _description_, by default None
    dim : int, optional
        _description_, by default 1
    """
    # get out of range error when evaluating at knots (other than 0)... not quite sure why this is.
    C = onp.zeros((len(cp_ordering),len(lagrange_points)))

    if SAVE_C_S:
        C_s = onp.zeros((len(N_u), len(onp.unique(lagrange_points[:,0])), dim))
    
    total_cp_combos = len(N_u)*len(N_v)*len(N_w)

    # represent tuples as ints, saved as floats accidentally
    cp_ordering = onp.array(cp_ordering.tolist(), dtype='int64')

    if len(cp_ordering) != total_cp_combos:
        raise Exception("only basis functions that are supported can be passed")

    # might need to come up with a way to identify         
    for l, point in enumerate(lagrange_points):
        count = 0    
        # we can enumerate in this manner as 
        # the product of the numbers of basis functions
        # will be the same length of the point orderings;
        # an error is thrown above if not.
        for i in range(len(N_u)):
            for j in range(len(N_v)):
                for k in range(len(N_w)):
                    # check if all basis functions are supported
                    # leads to a lot of unnecessary computation, but this is just for testing purposes.
                    ijk_tuple = cp_ordering[count]

                    N_u_i = N_u[ijk_tuple[0]](point[0], False)
                    N_v_j = N_v[ijk_tuple[1]](point[1], False)
                    N_w_k = N_w[ijk_tuple[2]](point[2], False)
                    product = N_u_i * N_v_j * N_w_k

                    # assigns this multiple times (like a lot...) but that's ok
                    # the * 8 is problem-specific and operator number specific
                    # but that's ok for now.
                    if SAVE_C_S:
                        C_s[ijk_tuple[0], int(point[0]*8), 0] = N_u_i
                        C_s[ijk_tuple[1], int(point[1]*8), 1] = N_v_j
                        C_s[ijk_tuple[2], int(point[2]*8), 2] = N_w_k


                    # save the tensor product of bspline basis fun evals
                    # to the extraction operator!
                    C[count][l] = product

                    count += 1
    if SAVE_C_S:
        return C, C_s
    else:
        return C

def create_bspline_basis_fun(knot, deg, extrapolate=False):

    t = knot

    # list of functions to return
    B = []

    t_arr = onp.array(t)    
    nonzero_knots = (t_arr != 0) & (t_arr != 1)
    num_interior_knots = onp.sum(nonzero_knots)
    for i in range(num_interior_knots + deg + 1):
        
        # print(t[i:deg + 2 + i]) # [u_i, u_{i+p+1})
        b = scipyBSpline.basis_element(t[i:deg + 2 + i], extrapolate=extrapolate)
        B.append(b)

    # return the list of functions - the user will need to pass the knot again
    # and the span of each basis function can thus be re-computed (although this is not needed)
    return B

# could maybe make this code more modular, but the functions all do slightly different things...
def eval_bspline(B, coeffs, eval_points, dim):
    """ evaluates a bspline function

    given a list of bspline basis functions B and corresponding coefficients (in 1D), evaluates
    the spline at eval_points

    Parameters
    ----------
    B : list
        list of basis functions
    coeffs : np.ndarray
        (len(B), )
    eval_points : np.ndarray
        (num_pts, )
    """
    
    b_eval = onp.zeros((len(eval_points), dim))
    
    for i, b in enumerate(B):
        # some entries will be NaN
        b_i_eval = onp.nan_to_num(b(eval_points, False))

        for j in range(dim):
            b_eval[:,j] += coeffs[i][j] * b_i_eval

    return b_eval

def eval_bspline_field(N_list, coeffs, eval_points, dim=1):
    """ evaluates a field constructed from bsplines

    _extended_summary_

    Parameters
    ----------
    N_list : list
        list of scipy.interpolate.BSpline.basis_element functions
    coeffs : _type_
        _description_
    eval_points : _type_
        _description_
    """

    # this only works in 3D for now; # TODO: abstract to n-dimensions
    N_u = N_list[0]
    N_v = N_list[1]
    N_w = N_list[2]

    grid_points = eval_points
    eval_field = onp.zeros((len(grid_points), dim))
    for l, point in enumerate(grid_points):
        for i in range(len(N_u)):
            for j in range(len(N_v)):
                for k in range(len(N_w)):
                    eval_field[l] += coeffs[i,j,k] * onp.nan_to_num(N_u[i](point[0], False)) * onp.nan_to_num(N_v[j](point[1], False)) * onp.nan_to_num(N_w[k](point[2], False))

    return eval_field

