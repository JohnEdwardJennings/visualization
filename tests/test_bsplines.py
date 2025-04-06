# Kenneth Meyer
# 3/28/25
# tests bspline evaluations.
# does not check for speed, tests for accuracy.

import unittest
import numpy as onp
import numpy.testing as onptest
import jax.numpy as np
import jax
import scipy as sp
from bspline_src.bspline_eval import naive_deboor

import matplotlib.pyplot as plt

jax.config.update("jax_enable_x64", True)

# test user-implemented bspline class
class TestBspline(unittest.TestCase):

    # def setUp(self):

    # automating testing of each function 
    # NEEDS to happen to streamline development.

    def test_naive_deboor_numpy(self):
        # use scipy bspline evaluation tool to test various cases of evaluation.
        # note: I'm not sure if scipy can handle unclamped knots? I'm a little confused...
        #       using an example from piegl and tiller, or computing a result by hand, is ideal here.
        
        z_min = 0.0
        P_0 = [1.0, 1.5, z_min]
        P_1 = [1.5, 1.0, z_min]
        P_2 = [2.0, 1.5, z_min]
        P_3 = [1.5, 2.0, z_min]
        P_4 = P_0
        P_5 = P_1
        P_6 = P_2
        # note: I'm not totally sure if points are going to be evenly spaced in physical space;
        #       pay attention to this when generating geometries in the future!
        control_points = onp.array([P_0, P_1, P_2, P_3, P_4, P_5, P_6])
        
        #knots = [[0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]]
        knots = [[-0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75]]
        
        
        degs = [3]
        
        # evaluate in 2D
        control_points = control_points[:,0:2]

        # define bspline curve using scipy.interpolate.BSpline
        # do x and y coordinates need to be evaluated separately?
        S_bpsline_x = sp.interpolate.BSpline(knots[0],control_points[:,0], degs[0])
        S_bpsline_y = sp.interpolate.BSpline(knots[0],control_points[:,1], degs[0])

        # evaluate curve using scipy objects and custom class
        eval_pts = onp.linspace(0.0,1.0,100)

        scipy_pts = onp.zeros((len(eval_pts), 2))
        custom_pts = onp.zeros((len(eval_pts), 2))
        
        for i in range(len(eval_pts)):
            
            # how does this translate to 3D?? I'm a tad confused...
            scipy_pts[i,0] = S_bpsline_x(eval_pts[i])
            scipy_pts[i,1] = S_bpsline_y(eval_pts[i])

            custom_pts[i,0] = naive_deboor(control_points[:,0], 
                                            knots, degs, eval_pts[i],
                                            [len(control_points)])
            custom_pts[i,1] = naive_deboor(control_points[:,1], 
                                            knots, degs, eval_pts[i],
                                            [len(control_points)])
            
        fig = plt.figure()
        ax = fig.add_subplot()
        ax.scatter(scipy_pts[:,0], scipy_pts[:,1])
        ax.scatter(custom_pts[:,0], custom_pts[:,1])
        ax.legend(["scipy", "custom"])
        plt.show()

        onptest.assert_allclose(scipy_pts, custom_pts)

    # should write better tests in CARDIAX so things are smaller...
    def test_naive_deboor_3D(self):
        # use scipy bspline evaluation tool to test various cases of evaluation.
        # note: I'm not sure if scipy can handle unclamped knots? I'm a little confused...
        #       using an example from piegl and tiller, or computing a result by hand, is ideal here.
        
        z_min = 0.0
        P_0 = [1.0, 1.5, z_min]
        P_1 = [1.5, 1.0, z_min]
        P_2 = [2.0, 1.5, z_min]
        P_3 = [1.5, 2.0, z_min]
        P_4 = P_0
        P_5 = P_1
        P_6 = P_2
        # note: I'm not totally sure if points are going to be evenly spaced in physical space;
        #       pay attention to this when generating geometries in the future!
        control_points = onp.array([P_0, P_1, P_2, P_3, P_4, P_5, P_6])
        
        #knots = [[0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]]
        knots = [[-0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75]]
        
        
        degs = [3]
        
        # evaluate in 2D
        # control_points = control_points[:,0:2]

        # define bspline curve using scipy.interpolate.BSpline
        # do x and y coordinates need to be evaluated separately?
        S_bpsline_x = sp.interpolate.BSpline(knots[0],control_points[:,0], degs[0])
        S_bpsline_y = sp.interpolate.BSpline(knots[0],control_points[:,1], degs[0])
        S_bpsline_z = sp.interpolate.BSpline(knots[0],control_points[:,2], degs[0])

        # evaluate curve using scipy objects and custom class
        eval_pts = onp.linspace(0.0,1.0,100)

        scipy_pts = onp.zeros((len(eval_pts), 3))
        custom_pts = onp.zeros((len(eval_pts), 3))
        
        for i in range(len(eval_pts)):
            
            # how does this translate to 3D?? I'm a tad confused...
            scipy_pts[i,0] = S_bpsline_x(eval_pts[i])
            scipy_pts[i,1] = S_bpsline_y(eval_pts[i])
            scipy_pts[i,2] = S_bpsline_z(eval_pts[i])

            custom_pts[i,0] = naive_deboor(control_points[:,0], 
                                            knots, degs, eval_pts[i],
                                            [len(control_points)])
            
            custom_pts[i,1] = naive_deboor(control_points[:,1], 
                                            knots, degs, eval_pts[i],
                                            [len(control_points)])
            
            custom_pts[i,2] = naive_deboor(control_points[:,2], 
                                            knots, degs, eval_pts[i],
                                            [len(control_points)])
            
        fig = plt.figure()
        ax = fig.add_subplot(projection='3d')
        ax.scatter(scipy_pts[:,0], scipy_pts[:,1], scipy_pts[:,2])
        ax.scatter(custom_pts[:,0], custom_pts[:,1], custom_pts[:,2])
        ax.legend(["scipy", "custom"])
        plt.show()

        onptest.assert_allclose(scipy_pts, custom_pts)

if __name__ == "__main__":
    unittest.main()