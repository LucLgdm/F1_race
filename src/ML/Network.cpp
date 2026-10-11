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
	std::vector<float> outputs = _layers.front().forward(inputs);
	for(std::size_t i = 1; i < _layers.size(); i++) {
		outputs = _layers[i].forward(outputs);
	}
	return outputs;
}
