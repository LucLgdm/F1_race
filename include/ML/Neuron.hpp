/* ****************************************** */
/*   Neuron.hpp                               */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 16:24:26 by Luc      */
/*   Updated: 2026/10/11 16:24:26 by Luc      */
/* ****************************************** */

#pragma once

#include <span>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstddef>
#include <random>
#include <cmath>

#include "Exception.hpp"
#include "Activation.hpp"
#include "Loss.hpp"

class Neuron {
	public:
		Neuron(std::vector<float> weights, float bias, Activation activation = Activation::ReLU);
		Neuron(std::size_t inputCount, Activation activation = Activation::Linear);

		// Compute the output of the neuron given a set of inputs
		// Span is used to avoid copying the input vector and to allow for flexible input sizes.
		[[nodiscard]] float forward(std::span<const float> inputs) const;

		void train(std::span<const float> inputs, float target, float learningRate);
		float weightedSum(std::span<const float> inputs) const;
		std::vector<float> backward(std::span<const float> inputs, float outputGradient, float learningRate);
		void mutate(float probability, float strength, std::mt19937& generator);

	private:
		std::vector<float> _weights;
		float _bias;
		Activation _activation;
	friend class ChampionIO;
};
