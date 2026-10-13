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

struct Sample {
	std::array<float, 1> inputs;
	float target;
};

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

void testTraining() {
	Neuron neuron{1, Activation::Linear};

	const std::array<Sample, 3> samples{{
		{{-1.0f}, -1.0f},
		{{ 0.0f},  1.0f},
		{{ 1.0f},  3.0f}
	}};

	const auto averageLoss = [&]() {
		float total = 0.0f;

		for (const Sample& sample : samples) {
			const float error = neuron.forward(sample.inputs) - sample.target;
			total += error * error;
		}
		return total / static_cast<float>(samples.size());
	};

	std::cout << "Initial loss: " << averageLoss() << std::endl;

	constexpr float learningRate = 0.05f;
	for (int epoch = 1; epoch <= 300; ++epoch) {
		for (const Sample& sample : samples) {
			neuron.train(sample.inputs, sample.target, learningRate);
		}
		if (epoch % 50 == 0)
			std::cout << "Epoch " << epoch << " | Loss: " << averageLoss() << std::endl;
	}

	// An input that was not used during training
	const std::array<float, 1> inputs{0.5f};
	std::cout << "Prediction for x = 0.5: " << neuron.forward(inputs) << " | Expected 2" << std::endl;
}