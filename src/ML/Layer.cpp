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

std::vector<float> Layer::forward(std::span<const float> inputs) const {
	std::vector<float> outputs;
	outputs.reserve(_neurons.size());

	for(const Neuron& neuron : _neurons) {
		outputs.push_back(neuron.forward(inputs));
	}
	return outputs;
}
