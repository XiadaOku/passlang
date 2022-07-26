#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "../src/passlang.h"


enum World {
	Fostral = 0,
	Glorx,
	Necross,
	Xplo,
	Khox,
	Boozeena,
	Weexow,
	Hmok,
	Threall,
	Arkonoy,
	size
};

int main(int argc, char** args) {
	int checksNumber = 7;
	std::cin >> checksNumber;

	std::string expression = "0-2 (n - 1)(-)";
	std::getline(std::cin >> std::ws, expression);

	auto getChecks = initPasslang([](int world, int x, int y) -> passlang::C_Check {
		if (world == passlang::randomPlaceholder) {
			do {
				world = rand() % World::size;
			} while (world == World::Hmok);
		}
		if (x == passlang::randomPlaceholder) {
			x = rand() % 2048;
		}
		if (y == passlang::randomPlaceholder) {
			y = rand() % 2048;
		}

		if (world < 0 || world >= (int)World::size) {
			throw std::runtime_error("world value out of bounds");
		}
		if (x < 0/* || x >= world_size_x*/) {
			throw std::runtime_error("x value out of bounds");
		}
		if (y < 0/* || y >= world_size_y*/) {
			throw std::runtime_error("y value out of bounds");
		}

		return {world, x, y};
	}, [](int start, int finish) -> int {
		return start + (std::rand() % (finish - start + 1));
	});

	std::srand(std::time(nullptr));

	std::vector<passlang::C_Check> results = getChecks(checksNumber, expression);

	for (auto i: results) {
		std::cout << i << " ";
	}
	std::cout << std::endl;

	return 0;
}
