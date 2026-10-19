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

void Neuron::train(std::span<const float> inputs, float target, float learningRate) {
	if (!std::isfinite(learningRate) || learningRate <= 0.0f)
		throw neuralError("Learning rate must be finite and positive");
	const float error = forward(inputs) - target;
	backward(inputs, 2.0f * error, learningRate);
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

std::vector<float> Neuron::backward(std::span<const float> inputs, float outputGradient, float learningRate) {
	const float z = weightedSum(inputs);
	const float delta = outputGradient * activationDerivative(z, _activation);
	std::vector<float> inputGradients(inputs.size());

	// Use the original weights to propragate the gradient
	for (std::size_t i = 0; i < inputs.size(); ++i) {
		inputGradients[i] = delta * _weights[i];
	}

	// Update the parameters
	for (std::size_t i = 0; i < inputs.size(); ++i) {
		_weights[i] -= learningRate * delta * inputs[i];
	}
	_bias -= learningRate * delta;

	return inputGradients;
}

/******************************************************************
 * 2 parameters controls the mutation : probability and strength :
 * the probabililty to upgrade the weigth or the bias
 * the standard deviation of the perturbations added
 ******************************************************************/

void Neuron::mutate(float probability, float strength, std::mt19937& generator) {
	if (!std::isfinite(probability) || probability < 0.0f
		|| probability > 1.0f || !std::isfinite(strength)
		|| strength < 0.0f)
		throw neuralError ("Invalid mutation parameters");
	
	if (probability == 0.0f || strength == 0.0f)
		return;

	std::bernoulli_distribution shouldMutate(probability);
	std::normal_distribution<float> perturbation(0.0f, strength);

	for (float& weight : _weights) {
		if (shouldMutate(generator))
			weight += perturbation(generator);
	}

	if (shouldMutate(generator))
		_bias += perturbation(generator);
}