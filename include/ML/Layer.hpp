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

		[[nodiscard]] std::vector<float> forward(std::span<const float> inputs) const;
	
	private:
		std::vector<Neuron> _neurons;
};
