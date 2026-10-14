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

void testNeuron();
void testLayer();
void testNetwork();
void testNeuralTraining();
void testNetworkTraining();

int main() {
	std::cout << "Hello F1 ML!" << std::endl;
	// Application app;
	
	try {
		// app.init();
		// app.run();
		Track track{120.0f, 80.0f, 10.0f};
		Car car{track.startPosition()};

		const auto distances = measureWallDistances(car, track, 40.0f);

		for (float distance : distances) {
			std::cout << distance << '\n';
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}

