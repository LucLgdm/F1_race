/* ****************************************** */
/*   Layer.cpp                                */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 19:19:18 by Luc      */
/*   Updated: 2026/10/11 19:19:18 by Luc      */
/* ****************************************** */

#include "ML/Layer.hpp"

Layer::Layer(std::vector<Neuron> neurons) : _neurons(std::move(neurons)) {
	if (_neurons.empty())
		throw neuralError("Layer must have at least one neuron.");
}

Layer::Layer(std::size_t inputCount, std::size_t neuronCount, Activation activation) {
	if (inputCount == 0 || neuronCount == 0)
		throw ("Layer dimensions must be positive");

	_neurons.reserve(neuronCount);
	for(std::size_t i = 0; i < neuronCount; ++i) {
		_neurons.emplace_back(inputCount, activation);
	}
}

std::vector<float> Layer::forward(std::span<const float> inputs) const {
	std::vector<float> outputs;
	outputs.reserve(_neurons.size());

	for(const Neuron& neuron : _neurons) {
		outputs.push_back(neuron.forward(inputs));
	}
	return outputs;
}

std::vector<float> Layer::backward(std::span<const float> inputs, std::span<const float> outputGradients, float learningRate) {
	if (outputGradients.size() != _neurons.size())
		throw neuralError("Each neuron must recieve one output gradient");

	std::vector<float> inputGradients(inputs.size(), 0.0f);

	for (std::size_t i = 0; i < _neurons.size(); ++i) {
		const auto contribution = _neurons[i].backward(inputs, outputGradients[i], learningRate);
		for(std::size_t j = 0; j < inputs.size(); ++j) {
			inputGradients[j] += contribution[j];
		}
	}
	return inputGradients;
}

void Layer::mutate(float probability, float strength, std::mt19937& generator) {
	for (Neuron& neuron : _neurons)
		neuron.mutate(probability, strength, generator);
}