#include <iostream>
#include <string>
#include <vector>
#include "main.h"


int main(int argc, char** args) {
	int checksNumber = 7;
	std::cin >> checksNumber;

	std::string expression = "0-2 (n - 1)(-)";
	std::getline(std::cin >> std::ws, expression);
	
	std::vector<passlang::C_Check> results = getChecks(checksNumber, expression);

	for (auto i: results) {
		std::cout << i << " ";
	}
	std::cout << std::endl;

	return 0;
}
