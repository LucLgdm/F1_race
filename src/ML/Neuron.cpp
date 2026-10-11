/* ****************************************** */
/*   Neuron.cpp                               */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 16:29:31 by Luc      */
/*   Updated: 2026/10/11 16:29:31 by Luc      */
/* ****************************************** */

#include "ML/Neuron.hpp"

Neuron::Neuron(std::vector<float> weights, float bias, Activation activation) 
			: _weights(std::move(weights)), _bias(bias), _activation(activation) {
	if (_weights.empty())
		throw neuralError("Neuron must have at least one weight.");
}

/************************************************************
 * forward compute the output after a linear transformation
 * followed by a ReLU activation function.
 * The linear transformation is defined as:
 * y = sum(0, N){xi * wi} + b
 ************************************************************/

float Neuron::forward(std::span<const float> inputs) const {
	if (inputs.size() != _weights.size())
		throw neuralError("Input size does not match the number of weights.");

	float sum = _bias;
	for(std::size_t i = 0; i < inputs.size(); ++i) {
		sum += inputs[i] * _weights[i];
	}
	return activate(sum, _activation);
}
