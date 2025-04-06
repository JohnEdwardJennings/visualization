#include "geometry.h"
#include <cstdio>
#include <iostream>

using namespace std;

void error(char const * msg) {
	printf("Error: %s\n", msg);
	exit(1);		
}

/* Tests */
int main() {
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
