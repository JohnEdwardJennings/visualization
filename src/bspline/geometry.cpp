#include "geometry.h"
#include <cstdio>
int main() {
	std::array<size_t,2> degrees{2,2};
	std::array<std::vector<double>, 2> knot_vectors{std::vector<double>{1, 2, 3, 4, 5}, std::vector<double>{1, 2, 3, 4, 5}}; 
	std::vector<std::array<double, 3>> control_points(25);
	BSplineGeometry<2,3> b(degrees, knot_vectors, control_points);
	std::vector<std::array<double,2>> x = {{0, 1}, {2, 3}};
	//b.evaluate(x);
	return 0;
}
