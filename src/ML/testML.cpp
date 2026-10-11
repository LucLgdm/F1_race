/* ****************************************** */
/*   testML.cpp                         */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 19:31:02 by Luc      */
/*   Updated: 2026/10/11 19:31:02 by Luc      */
/* ****************************************** */

#include <iostream>

#include "ML/Network.hpp"
#include "ML/Loss.hpp"

void testNeuron() {
	// Neuron neuron{{0.4f, -0.2f}, 0.1f};
	Neuron neuron{{1.0f}, 0.0f, Activation::Tanh};
	// const std::array<float, 2> inputs = {0.5f, 0.8f};
	const std::array<float, 1> inputs = {-0.5f};
	std::cout << "Neuron output: " << neuron.forward(inputs) << std::endl;
}

void testLayer() {
	Layer layer{
		std::vector<Neuron>{
			Neuron{{ 0.4f, -0.2f},  0.1f},
			Neuron{{ 0.1f,  0.6f}, -0.2f},
			Neuron{{-0.5f,  0.3f},  0.0f}
		}
	};
	const std::array<float, 2> inputs = {0.5f, 0.8f};
	std::vector<float> outputs = layer.forward(inputs);
	std::cout << "Layer outputs: ";
	for (float output : outputs) {
		std::cout << output << " ";
	}
	std::cout << std::endl;
}

void testNetwork() {
	Network network{
		std::vector<Layer>{
			Layer{
				std::vector<Neuron>{
					Neuron{{ 0.4f, -0.2f},  0.1f},
					Neuron{{ 0.1f,  0.6f}, -0.2f},
					Neuron{{-0.5f,  0.3f},  0.0f}
				}
			},
			Layer{
				std::vector<Neuron>{
					Neuron{{0.5f, -0.4f, 0.2f}, 0.1f}
				}
			}
		}
	};

	const std::array<float, 2> inputs{0.5f, 0.8f};

	const auto outputs = network.forward(inputs);

	std::cout << "Network output: " << outputs.front() << std::endl;

	const std::array<float, 1> targets{0.5f};
	const float loss = meanSquaredError(outputs, targets);
	std::cout << "Loss: " << loss << std::endl;
}

