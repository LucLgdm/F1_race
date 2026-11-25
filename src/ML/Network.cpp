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
std::vector<std::size_t> Network::topology() const {
    std::vector<std::size_t> shape{_layers.front()._neurons.front()._weights.size()};
    for (const auto& layer : _layers) shape.push_back(layer._neurons.size());
    return shape;
}
std::vector<float> Network::parameters() const {
    std::vector<float> values;
    for (const auto& layer : _layers) for (const auto& n : layer._neurons) {
        values.insert(values.end(),n._weights.begin(),n._weights.end());
        values.push_back(n._bias);
    }
    return values;
}
void Network::forwardInto(std::span<const float> inputs, std::span<float> output,
                         std::span<float> a, std::span<float> b) const {
    std::span<const float> current=inputs;
    for (std::size_t i=0;i<_layers.size();++i) {
        const auto& layer=_layers[i];
        auto target=(i%2==0 ? a : b);
        if (target.size()<layer._neurons.size()) throw neuralError("Inference workspace too small");
        for (std::size_t j=0;j<layer._neurons.size();++j) target[j]=layer._neurons[j].forward(current);
        current=target.first(layer._neurons.size());
    }
    if (output.size()!=current.size()) throw neuralError("Invalid inference output size");
    std::copy(current.begin(),current.end(),output.begin());
}
Network Network::expandedDrivingNetwork() const {
    const auto shape=topology();
    if (shape.size()!=4 || (shape[0]!=8 && shape[0]!=23) || shape[1]>32 || shape[2]>32 || shape[3]!=3)
        throw neuralError("Transfer requires a driving network with two hidden layers of at most 32 neurons");
    const std::size_t widths[4]={23,32,32,3};
    std::vector<Layer> layers;
    for (std::size_t l=0;l<3;++l) {
        std::vector<Neuron> neurons;
        for (std::size_t j=0;j<widths[l+1];++j) {
            std::vector<float> weights(widths[l],0.0f);
            float bias=0.0f;
            Activation activation=Activation::Tanh;
            if (j<_layers[l]._neurons.size()) {
                const auto& source=_layers[l]._neurons[j];
                std::copy(source._weights.begin(),source._weights.end(),weights.begin());
                bias=source._bias; activation=source._activation;
            }
            neurons.emplace_back(std::move(weights),bias,activation);
        }
        layers.emplace_back(std::move(neurons));
    }
    return Network{std::move(layers)};
}
