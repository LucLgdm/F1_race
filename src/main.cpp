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
	std::cout << "Hello World!" << std::endl;
	Application app;

	app.init();
	app.run();

	return 0;
}
