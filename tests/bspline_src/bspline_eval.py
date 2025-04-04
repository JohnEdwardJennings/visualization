# Kenneth Meyer
# 3/28/25
# fast evaluation of BSplines, open and closed.

import numpy as onp
import jax.numpy as np

# de boor's algorithm, allowing for evaluation with clamped and unclamped knot
# vectors.
# does not attempt to exploit tensor product structure nor store values
def naive_deboor(cps, knots, degs, x, n_cp_list, save_row=False):
    """ naive implementation of de boor's algorithm

    Uses: https://tiborstanko.sk/teaching/geo-num-2017/tp3.html

    Parameters
    ----------
    cps : _type_
        control points
    knots : _type_
        knot vectors
    degs : _type_
        degree in each parametric direction
    x : _type_
        evaluation points
    n_cp_list : list
        number of control points in each parametric direction
    """

    # assertions to make sure this function isn't misused!
    # print("cp len: " + str(len(cps)))
    # print("n cp: " + str(onp.product(onp.array(n_cp_list))))
    assert len(n_cp_list) == len(knots)
    assert len(cps) == onp.product(onp.array(n_cp_list))

    # quickly check dimensions of objects
    # need to check for each parametric direction...
    for i, deg in enumerate(degs):
        assert deg == len(knots[i]) - n_cp_list[i] - 1

    # hardcoded for 1D
    # not sure why this is throwing an error?
    # d_list = onp.ones((len(cps), degs[0]+1)) # need to check what the shape of this should be
    # making unnecessarily large for now...
    d_list = onp.ones((len(cps), degs[0]+1)) # need to check what the shape of this should be
   
    # only do this for a single input x for now... and in a single direction...
    m = n_cp_list[0] # assumes one direction for now!
    assert m == len(cps)

    # degree
    k = deg

    # determine which knot span x lives in. for loop is likely NOT the way to do this.
    i = -1
    # print(f"x: {x}")
    for ii in range(len(knots[0]) - 1):
        # might not be able to handle repeated knots

        # need to figure out how to handle the last knot interval appropriately
        if (knots[0][ii] <= x) & (knots[0][ii+1] > x):
            i = ii
            # try forcing this... seems to work?
            if i == len(cps):
                i = i-1
            break
        # this seems like it can handle the case where x is on the right boundary of the domain
        if x == 1.0:
            i = len(cps) - 1
            break

    # throw an error message if the point is out of the knot span
    if i == -1:
        raise Exception("knot is out of knot span! (or lands on a repeated knot...)")

    # again, we're only doing this in one parametric direction...
    t = knots[0]

    # provide control points in each knot interval
    # 
    # if we're dealing with lagrange extraction, cps are all 1 as extraction
    # is defined for cells on a unit cube mesh. Control points warp geometries AFTER
    # extraction operators are generated. 
    for j in range(i-k, i+1):
        d_list[j,0] = cps[j]

    # looks like this evaluation code works! great news. lots of issues with 
    for r in range(1, k+1):
        for j in range(i-k+r, i+1):
            # come up with a way to handle division without using nan_to_num...
            # should hopefully avoid unnecessary computation.
            # can specialize this for extraction operator generation eventually...
            alpha = onp.nan_to_num((x - t[j]) / (t[j + k - r + 1] - t[j]))
            d_list[j,r] = (1 - alpha) * d_list[j-1,r-1] + alpha * d_list[j, r-1]

    # provide an option to return the last row of d, which might be required for christian's bspline
    # evaluation tool...
    if save_row:
        return d_list[i]
    else:
        return d_list[i,k]

# def matrix_deboor_numpy():


#def mapped_deboor_jax():



# need to figure out a good way to structure this project
if __name__ == "__main__":
    # might be a good idea to run some tests here...
    print("BSpline functions")