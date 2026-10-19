/* ****************************************** */
/*   Layer.hpp                                */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 16:43:41 by Luc      */
/*   Updated: 2026/10/11 16:43:41 by Luc      */
/* ****************************************** */

#pragma once

#include <vector>
#include <span>

#include "ML/Neuron.hpp"

class Layer {
	public:
		explicit Layer(std::vector<Neuron> neurons);
		Layer(std::size_t inputCount, std::size_t neuronCount, Activation activation);

		[[nodiscard]] std::vector<float> forward(std::span<const float> inputs) const;

		std::vector<float> backward(std::span<const float> inputs, std::span<const float> outputGradients, float learningRate);
		void mutate(float probability, float strength, std::mt19937& generator);
	
	private:
		std::vector<Neuron> _neurons;
};
