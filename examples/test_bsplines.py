"""

    Test various BSpline-related CARDIAX functions.

"""

import numpy as onp
import numpy.testing as onptest
import jax
import jax.numpy as np
import meshio
import os
import unittest

from cardiax.generate_mesh import box_mesh
from cardiax.problem import Problem
from cardiax.fe import FiniteElement
from cardiax.solver import Newton_Solver
from cardiax.post_process import get_F
from cardiax.iga import BSpline
from cardiax.unittest_helpers import create_bspline_basis_fun, eval_bspline, eval_bspline_field

# the functions we will be testing:
from cardiax.generate_mesh import box_mesh_bspline, rect_prism_bspline,convert_geomdl_info

jax.config.update("jax_enable_x64", True)

class subTest(unittest.TestCase):

    def setUp(self):
        # sets up the tests to run on quadratic BSplines.
        # might want to iterate tests over multiple inputs...
        self.vec = 1                                # scalar or vector-valued
        self.degrees = [2,3]                             # degree of basis function

    def create_scipy_bspline(self, fe):
        """
        """
        # basis func in u
        N_u = create_bspline_basis_fun(fe.knots[0], fe.degrees[0])
        
        # basis func in v
        N_v = create_bspline_basis_fun(fe.knots[1], fe.degrees[1])
        
        # basis func in w
        N_w = create_bspline_basis_fun(fe.knots[2], fe.degrees[2])
        
        return N_u, N_v, N_w

    def eval_scipy_bspline_ele(self, N_u, N_v, N_w, grid_points, control_points, deg, dim=1):
        eval_field = onp.zeros((len(grid_points),dim))
        
        # there are deg + 1 basis functions in each direction!
        #control_points = control_points.reshape((deg + 1, deg + 1, deg + 1, dim))

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
                            # also print stuff to see it
                            # print(f"N_u({point[0]}) : {N_u_i}")
                            # print(f"N_v({point[1]}) : {N_v_j}")
                            # print(f"N_w({point[2]}) : {N_w_k}")
                            print(f"product: {product}")
        return eval_field
        

    def get_ele_type(self, deg):
        ele_type='SPLINEHEX' + str(deg) # element type to pass to CARDIAX
        return ele_type
    
    def get_key(self, deg):
        # element type, dependent on the degree of the bsplines
        if deg == 1:
            key = 'hexahedron'
        elif deg == 2:
            key = 'hexahedron27'
        elif deg == 3:
            key = 'hexahedron64'
        else:
            raise NotImplementedError
        return key
    
    def reorder_points(self, points, deg):
        """ reorders points according to shuffling

        points get shuffled when using meshio/paraview node orderings vs. iga-based node orderings

        """

        # relies on self.degree to apply the appropriate re-ordering
        if deg == 1:
            re_order = [0, 1, 3, 2, 4, 5, 7, 6]
        elif deg == 2:
            raise NotImplementedError
        else:
            raise NotImplementedError
        
        points = points[:,re_order]
        return points

    def generate_cardiax_BSpline_obj(self, Nx, Ny, Nz, vec, deg, Lx=None, Ly=None, Lz=None, control_points=None):
        """ generates a :py:class:`cardiax.iga.BSpline` instance

        generates a BSpline mesh using CARDIAX-only functions
        """

        # reparameterize the cube to form a rectangle
        if Lx is not None:
            if (Ly is not None) and (Lz is not None):
                reparam=onp.array([Lx,Ly,Lz])
            else:
                raise Exception("You need to pass lengths in all 3 directions!")
        else:
            reparam=None

        # reparameterize the cube using control points - just need to throw error if needed
        if control_points is not None:
            if reparam is not None:
                raise Exception("Can't reparameterize using scaling AND control points!! Choose only 1.")

        ele_type = 'SPLINEHEX'+str(deg)
        knot0 = np.hstack((np.zeros(deg), np.linspace(0,1,Nx + 1) ,np.ones(deg)))
        knot1 = np.hstack((np.zeros(deg), np.linspace(0,1,Ny + 1) ,np.ones(deg)))
        knot2 = np.hstack((np.zeros(deg), np.linspace(0,1,Nz + 1) ,np.ones(deg)))
        knots = [knot0, knot1, knot2]
        degrees = 3*[deg]
        fe = BSpline(knots, degrees, vec = vec, dim = 3, ele_type = ele_type, gauss_order = deg, 
                    reparam=reparam, control_points=control_points)
        return fe

    def test_geomdl_unit_cube(self):
        # code goes here!
        print('aaa')

        # things to test:
        # 1. if a geomdl-based mesh using control points and a reparameterized
        #    MESH are the same (and a non-parameterized mesh, just a unit cube)
        # objects to test:
        #   1a. lagrange meshes generated
        #   1b. the bspline-based mesh?
        # 2. if the solutions obtained using each mesh are the same
        #    should obtain the exact same solution...
        # other things to test:
        #   NEED TO TEST IF THE FOLLOWING WORK:
        #   3. boundary conditions - check if location functions work as intended with reparam and control points
        #   4. vector-valued and scalar-valued problems

        Nx, Ny, Nz = 4,4,4
        Lx, Ly, Lz = 1,1,1

        # test that the application of control points is correct for each degree
        for deg in self.degrees:

            ele_type = self.get_ele_type(deg)

            geomdl_box = box_mesh_bspline(Nx, Ny, Nz, Lx, Ly, Lz, deg)

            # add sub-tests to check if this works for scalar and vector-valued functions
            # might actually want to compare other quantities/might be out of scope.

            # almost everything needs to be re-formatted, this is done by a function
            knots, control_points, degs = convert_geomdl_info(geomdl_box)
            fe_geomdl = BSpline(knots, degs, vec = self.vec, dim = 3, ele_type = ele_type, gauss_order = deg, 
                        control_points=control_points)

            ## generate a BSpline mesh using the CARDIAX-only approach
            fe_cardiax = self.generate_cardiax_BSpline_obj(Nx, Ny, Nz, 
                                self.vec, deg, Lx=Lx, Ly=Ly, Lz=Lz, 
                                control_points=None)
        
            # check that the points that define each cell are the same!!
            #onptest.assert_allclose(fe_geomdl.points, fe_cardiax.points)

            # the ordering seems to be ok, but the actual values are off...
            #onptest.assert_allclose(fe_geomdl.S_ele, fe_cardiax.points[fe_cardiax.cells])
            
            # above,
            # I was trying to compare eval points on the physical mesh to
            # lagrange points! those do not always correspond!
            
            # the points don't correspond to greville abscissae ! so idk how to make this comparison...
            # EVALUATING GEOMDL VOLUMES at lagrange points and comparing to what is computed by CARDIAX
            # is a GREAT form of a unit test. We can do that and then move onto solving poisson on
            # a rectangle!!!!
            
            # this is failing. TODO: compare results with bsplines defined in scipy;
            #                   there should be code that does this.
            
            # evaluate the defined bspline using the post-processing functionality
            # provided by scipy-based routines
            for i in range(len(fe_geomdl.S_ele)):
                # evaluate the geomdl geometry at the same points that the mesh is evaluated at
                # via control points

                # geomdl_eval_at_lagrange = geomdl_box.evaluate_list(fe_cardiax.points[fe_cardiax.cells[i]])            
                
                # # this is extremely slow, but for each bspline element, evaluate the physical coordinate
                # # at given lagrange points using scipy. need to be careful with values near 1.
                # N_u, N_v, N_w = self.create_scipy_bspline(fe_cardiax)
                # grid_points_i = fe_geomdl.points[fe_geomdl.cells[i]]

                # # control points should be ordered correctly (increasing in z first, then y, then x)
                # # but will need to verify this.
                # control_points_i = fe_geomdl.control_points[fe_geomdl.domain.support[i]]

                # # bug in scipy.basis_element, points at end of knot span eval to 0
                # mat = control_points_i > 0.0
                # control_points_i[mat] = control_points_i[mat] - 1e-12

                # # evaluate the basis functions at the grid points

                # eval_at_lagrange = self.eval_scipy_bspline_ele(N_u, N_v, N_w, grid_points_i, control_points_i,
                #                                                deg, dim=3)
                
                # this was failing
                #onptest.assert_allclose(fe_geomdl.S_ele[i], geomdl_eval_at_lagrange)
                
                # this is...
                #onptest.assert_allclose(fe_geomdl.S_ele[i], eval_at_lagrange)
            

                onptest.assert_allclose(fe_geomdl.S_ele[i], fe_geomdl.points[fe_geomdl.cells[i]])
            

            #onptest.assert_allclose(fe_geomdl.S_ele, fe_cardiax.nodes[fe_cardiax.domain.support])
            

            #onptest.assert_allclose(fe_geomdl.points[fe_geomdl.cells], fe_cardiax.points[fe_geomdl.cells[:,[0, 1, 3, 2, 4, 5, 7, 6]]])



        # this needs to be moved to a subtest! things seem to be working for a unit cube and linear elements.
        # also checks if the same is true after re-parameterization
        # might want to move this to a subTest or something...
        # fe_cardiax_reparam = self.generate_cardiax_BSpline_obj(Nx, Ny, Nz, 
        #                     self.vec, self.degree, Lx=1.0, Ly=1.0, Lz=4.0, 
        #                     control_points=None)
        
        
        #onptest.assert_allclose(fe_geomdl.points, fe_cardiax_reparam.points)

    def test_cp_solution(self):
        """ tests control point solution is correct

        verifies that the solutions obtained using 'reparam' and by using control points
        are the same.

        Can also compare to an analytical solution, compare to linear hexes, or choose another
        approach. Choosing one of these might be beneficial as technically both the reparam
        and control point approaches could be wrong!
        """
        
    @unittest.skip("Not implemented yet")
    def test_geomdl_rect_prism(self):
        
        # preserve aspect ratio for each element in the mesh
        Nx, Ny, Nz = 4,4,16
        Lx, Ly, Lz = 1,1,4

    @unittest.skip("Not implemented yet")
    def test_periodic_knot(self):
        # code here
        print('aaaa')

        # tests if a mesh defined using periodic knot vectors:
        # 1. continuous at the connection (in lagrange space) throughout a simulation
        # 2. can be used in a simulation
        #
        # test problem:
        #      a hollow cylinder. Using thin shell theory to compare numerical simulations
        #      to analytical solutions is a good way of testing if we can solve these
        #      kinds of problems.

if __name__ == '__main__':
    
    # automatically run all tests
    unittest.main()

