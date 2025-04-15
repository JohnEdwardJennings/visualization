#include "geometry.h"
#include <cstdio>
#include <iostream>

using namespace std;

void error(char const * msg) 
{
	printf("Error: %s\n", msg);
	exit(1);		
}

/* Tests */
int main() 
{
	/* hardcoded test cases */

	{
		/* n_kdims = 1, n_cdims = 1, degree = 0 */
		std::array<size_t, 1> degrees{0};
		std::vector<double> _knots{0, 0.5, 1, 1, 1, 1, 1, 1};
		std::array<std::vector<double>, 1> knots{_knots};
		std::vector<std::array<double, 1>> control_points{{0}, {0.5}, {-1}, {-2}, {-3}, {-4}, {-5}};
		auto spline = BSplineGeometry<1,1>(degrees, knots, control_points);
		
		std::vector<std::array<double, 1>> x{{0}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			cout << (*i)[0] << " ";		
		}
		cout << "\n";
	}
	
	{
		/* n_kdims = 1, n_cdims = 2, degree = 0 */
		std::array<size_t, 1> degrees{0};
		std::vector<double> _knots{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1};
		std::array<std::vector<double>, 1> knots{_knots};
		std::vector<std::array<double, 2>> control_points{{0, 1}, {0.125, 2}, {0.25, 4}, {0.375, 8}, {0.5, 16}, {0.625, 32}, {0.75, 64}, {0.875, 128}};
		auto spline = BSplineGeometry<1,2>(degrees, knots, control_points);
		
		std::vector<std::array<double, 1>> x{{0}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			cout << "(" << (*i)[0] << ", " << (*i)[1] << ")" << " ";		
		}
		cout << "\n";
	}

	{
		/* n_kdims = 2, n_cdims = 2, degree = 0 */
		std::array<size_t, 2> degrees{0, 0};
		std::vector<double> x_knots{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1};
		std::vector<double> y_knots{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1};
		std::array<std::vector<double>, 2> knots{x_knots, y_knots};
		std::vector<std::array<double, 2>> control_points {	
			{0, 1}, {0.125, 2}, {0.25, 4}, {0.375, 8}, {0.5, 16}, {0.625, 32}, {0.75, 64}, {0.875, 128},
			{0, 1.1}, {0.125, 2.1}, {0.25, 4.1}, {0.375, 8.1}, {0.5, 16.1}, {0.625, 32.1}, {0.75, 64.1}, {0.875, 128.1},
			{0, 1.2}, {0.125, 2.2}, {0.25, 4.2}, {0.375, 8.2}, {0.5, 16.2}, {0.625, 32.2}, {0.75, 64.2}, {0.875, 128.2},
			{0, 1.3}, {0.125, 2.3}, {0.25, 4.3}, {0.375, 8.3}, {0.5, 16.3}, {0.625, 32.3}, {0.75, 64.3}, {0.875, 128.3},
			{0, 1.4}, {0.125, 2.4}, {0.25, 4.4}, {0.375, 8.4}, {0.5, 16.4}, {0.625, 32.4}, {0.75, 64.4}, {0.875, 128.4},
			{0, 1.5}, {0.125, 2.5}, {0.25, 4.5}, {0.375, 8.5}, {0.5, 16.5}, {0.625, 32.5}, {0.75, 64.5}, {0.875, 128.5},
			{0, 1.6}, {0.125, 2.6}, {0.25, 4.6}, {0.375, 8.6}, {0.5, 16.6}, {0.625, 32.6}, {0.75, 64.6}, {0.875, 128.6},
			{0, 1.7}, {0.125, 2.7}, {0.25, 4.7}, {0.375, 8.7}, {0.5, 16.7}, {0.625, 32.7}, {0.75, 64.7}, {0.875, 128.7}
		};
		auto spline = BSplineGeometry<2,2>(degrees, knots, control_points);
		
		std::vector<std::array<double, 2>> x{{0, 0}, {0.25, 0.25}, {0.5, 0.5}, {0.75, 0.75}, {1, 1}, 
			{0.125, 0.999}, {0.376, 0.75}, {0.5, 0.626}, {0.999, 0.375}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			cout << "(" << (*i)[0] << ", " << (*i)[1] << ")" << " ";		
		}
		cout << "\n";
	}

	{
		/* n_kdims = 3, n_cdims = 3, degree = 0 */
		std::array<size_t, 3> degrees{0, 0, 0};
		std::vector<double> x_knots{0, 0.33, 0.67, 1};
		std::vector<double> y_knots{0, 0.33, 0.67, 1};
		std::vector<double> z_knots{0, 0.33, 0.67, 1};
		std::array<std::vector<double>, 3> knots{x_knots, y_knots, z_knots};
		std::vector<std::array<double, 3>> control_points {	
			{100, 10, 1}, {100, 10, 2}, {100, 10, 3}, {100, 20, 1}, {100, 20, 2}, {100, 20, 3}, {100, 30, 1}, {100, 30, 2}, {100, 30, 3},
			{200, 10, 1}, {200, 10, 2}, {200, 10, 3}, {200, 20, 1}, {200, 20, 2}, {200, 20, 3}, {200, 30, 1}, {200, 30, 2}, {200, 30, 3},
			{300, 10, 1}, {300, 10, 2}, {300, 10, 3}, {300, 20, 1}, {300, 20, 2}, {300, 20, 3}, {300, 30, 1}, {300, 30, 2}, {300, 30, 3}
		};
		auto spline = BSplineGeometry<3,3>(degrees, knots, control_points);
		
		std::vector<std::array<double, 3>> x{{1, 0.5, 0}, {0.5, 0, 1}, {0, 1, 0.5}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			cout << "(" << (*i)[0] << ", " << (*i)[1] << ", " << (*i)[2] <<  ")" << " ";		
		}
		cout << "\n";
	}
	
	{
		/* n_kdims = 1, n_cdims = 1, degree = 1 */
		std::array<size_t, 1> degrees{1};
		std::vector<double> _knots{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1};
		std::array<std::vector<double>, 1> knots{_knots};
		std::vector<std::array<double, 1>> control_points{{1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}};
		auto spline = BSplineGeometry<1,1>(degrees, knots, control_points);
		
		std::vector<std::array<double, 1>> x{{0}, {0.125}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			cout << (*i)[0] << " ";		
		}
		cout << "\n";
	}
	
	{
		/* n_kdims = 1, n_cdims = 1, degree = 2 */
		std::array<size_t, 1> degrees{2};
		std::vector<double> _knots{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1};
		std::array<std::vector<double>, 1> knots{_knots};
		std::vector<std::array<double, 1>> control_points{{1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}};
		auto spline = BSplineGeometry<1,1>(degrees, knots, control_points);
		
		std::vector<std::array<double, 1>> x{{0}, {0.125}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			cout << (*i)[0] << " ";		
		}
		cout << "\n";
	}
	
	{
		/* n_kdims = 1, n_cdims = 1, degree = 10 */
		std::array<size_t, 1> degrees{10};
		std::vector<double> _knots{0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1};
		std::array<std::vector<double>, 1> knots{_knots};
		std::vector<std::array<double, 1>> control_points{{1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}};
		auto spline = BSplineGeometry<1,1>(degrees, knots, control_points);
		
		std::vector<std::array<double, 1>> x{{0}, {0.125}, {0.25}, {0.5}, {0.75}, {1}};
		
		auto y = spline.evaluate(x);
		
		for (auto i = y.begin(); i != y.end(); i++) {
			cout << (*i)[0] << " ";		
		}
		cout << "\n";
	}
}
