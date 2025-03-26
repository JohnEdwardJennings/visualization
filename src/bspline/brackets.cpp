#include <vector>
#include <iostream>
#include <cstddef>

int main() {
	std::vector<int> p{0, 1, 2, 3, 4, 5};
	for (size_t i = 0; i < 4; i++) {
		std::cout << p[i];
	}
	return 0;
}
