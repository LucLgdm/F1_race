/* ****************************************** */
/*   Network.cpp                              */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 21:58:43 by Luc      */
/*   Updated: 2026/10/11 21:58:43 by Luc      */
/* ****************************************** */

#include "ML/Network.hpp"

Network::Network(std::vector<Layer> layers) : _layers(std::move(layers)) {
	if (_layers.empty())
		throw neuralError("Network must have at least one layer.");
}

/************************************************
 * The current layer reads the previous results
 * then returns new results
 ************************************************/

std::vector<float> Network::forward(std::span<const float> inputs) const {
	// First layer
	std::vector<float> outputs = _layers.front().forward(inputs);
	for(std::size_t i = 1; i < _layers.size(); i++) {
		outputs = _layers[i].forward(outputs);
	}
	return outputs;
}

float Network::train(std::span<const float> inputs, std::span<const float> targets, float learningRate) {
	if (!std::isfinite(learningRate) || learningRate <= 0.0f)
		throw neuralError("Learning rate must be finite or positive");

	// activations[0] contains the network input
	// activations[i] contains layer i's outputs
	std::vector<std::vector<float>> activations;
	activations.reserve(_layers.size() + 1);
	activations.emplace_back(inputs.begin(), inputs.end());

	for (const Layer& layer : _layers) {
		activations.emplace_back(layer.forward(activations.back()));
	}
	const auto& predictions = activations.back();

	const float loss = meanSquaredError(predictions, targets);
	std::vector<float> gradients(predictions.size());
	const float scale = 2.0f / static_cast<float>(predictions.size());

	for (std::size_t i = 0; i < predictions.size(); ++i) {
		gradients[i] = scale * (predictions[i] - targets[i]);
	}

	// Visit the first layer then move backwards
	for (std::size_t i = _layers.size(); i > 0 ; --i) {
		gradients = _layers[i - 1].backward(activations[i - 1], gradients, learningRate);
	}

	return loss;
}

void Network::mutate(float probability, float strength, std::mt19937& generator) {
	for (Layer& layer : _layers)
		layer.mutate(probability, strength, generator);
}