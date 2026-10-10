/* ****************************************** */
/*   main.cpp                         */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:22:12 by Luc      */
/*   Updated: 2026/10/05 15:22:12 by Luc      */
/* ****************************************** */

#include <iostream>

#include "Application.hpp"

int main() {
	std::cout << "Hello F1 ML!" << std::endl;
	Application app;

	try {
		app.init();
		app.run();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
