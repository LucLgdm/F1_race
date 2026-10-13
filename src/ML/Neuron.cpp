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

/*******************************
 * generator produce a sequence of integer number wich seems to be random but in reality they are determined by an initial value : the seed
 * std::random_device{}() returns a random value
 * std::uniform_real_distribution<float> defines how the the random numbers have to be distributed
 ******************************/

Neuron::Neuron(std::size_t inputCount, Activation activation)
		: _weights(inputCount), _bias(0.0f), _activation(activation) {
	if (inputCount == 0)
		throw neuralError("Neuron must have at least one input.");
	
	std::mt19937 generator{std::random_device{}()};
	std::uniform_real_distribution<float> distribution(-0.5f, 0.5f);
	for (float& weight : _weights) {
		weight = distribution(generator);
	}
}

/************************************************************
 * forward compute the output after a linear transformation
 * followed by a ReLU activation function.
 * The linear transformation is defined as:
 * y = sum(0, N){xi * wi} + b
 ************************************************************/

float Neuron::forward(std::span<const float> inputs) const {
	return activate(weightedSum(inputs), _activation);
}

void Neuron::train(std::span<const float> inputs, float target, float learningrate) {
	if (!std::isfinite(learningrate) || learningrate <= 0.0f)
		throw neuralError("Learning rate must be finite and positive");

	const float z = weightedSum(inputs);
	const float prediction = activate(z, _activation);
	const float error = prediction - target;

	const float zGradient = 2.0f * error * activationDerivative(z, _activation);

	for (std::size_t i = 0; i < _weights.size(); ++i) {
		const float weightGradient = zGradient * inputs[i];
		_weights[i] -= learningrate * weightGradient;
	}

	_bias -= learningrate * zGradient;
}

float Neuron::weightedSum(std::span<const float> inputs) const {
	if (inputs.size() != _weights.size())
		throw neuralError("Each input must have a corresponding weight");

	float z = _bias;
	for(std::size_t i = 0; i < inputs.size(); ++i) {
		z += _weights[i] * inputs[i];
	}
	return z;
}