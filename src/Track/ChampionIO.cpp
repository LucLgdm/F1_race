/* ****************************************** */
/*   ChampionIO.cpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/20 14:53:11 by Luc      */
/*   Updated: 2026/10/20 14:53:11 by Luc      */
/* ****************************************** */

#include "Track/ChampionIO.hpp"

namespace {
	const char* activationName(Activation activation) {
		switch (activation) {
			case Activation::Linear: return "Linear";
			case Activation::ReLU:   return "ReLU";
			case Activation::Tanh:   return "Tanh";
		}
		throw IOError ("Unknown activation");
	}

	Activation readActivation(std::istream& input) {
		std::string name;

		if (!(input >> name))
			throw IOError("Missing activation");
			
		if (name == "Linear") return Activation::Linear;
		if (name == "ReLU")   return Activation::ReLU;
		if (name == "Tanh")   return Activation::Tanh;

		throw IOError("Unknown saved activation");
	}

	void expect(std::istream& input, const char* expected) {
		std::string token;

		if (!(input >> token) || token != expected)
			throw IOError("Invalid champion file structure");
	}

	float readFloat(std::istream& input) {
		float value;

		if (!(input >> value) || !std::isfinite(value))
			throw IOError("Invalid saved numeric value");
		return value;
	}

	std::size_t readCount(std::istream& input, std::size_t maximum) {
		long long value;

		if (!(input >> value) || value <= 0 || static_cast<unsigned long long> (value) > maximum)
			throw IOError("Invalid saved dimension");
		return static_cast<std::size_t>(value);
	}
}

void ChampionIO::write(std::ostream& output, const Track& track, const Network& network, float lapTime) {
	if (!std::isfinite(lapTime) || lapTime <= 0.0f)
		throw IOError("Invalid champion lap time");
	
	output.imbue(std::locale::classic());
	output << std::setprecision(std::numeric_limits<float>::max_digits10);

	output << "F1_CHAMPION 1" << std::endl;
	output << "RECTANGLE " 
			<< track._width << ' '
			<< track._height << ' '
			<< track._roadWidth << std::endl;

	output << "OBSERVATIONS 1 40 50" << std::endl;
	output << "CONTROLS 1" << std::endl;

	output << "LAP_TIME " << lapTime << std::endl;
	output << "LAYERS " << network._layers.size() << std::endl;

	for (const Layer& layer : network._layers) {
		output << "NEURONS " << layer._neurons.size() << std::endl;
		for (const Neuron& neuron : layer._neurons) {
			output << "NEURON "
			<< activationName(neuron._activation) << ' '
			<< neuron._bias << ' '
			<< neuron._weights.size();
			
			for (float weight : neuron._weights)
			output << ' ' << weight;
			
			output << std::endl;
		}
	}
	output << "END" << std::endl;
	output.flush();

	if (!output)
		throw IOError("Cannot write champion");
}

SavedChampion ChampionIO::read(std::istream& input) {
	input.imbue(std::locale::classic());

	expect(input, "F1_CHAMPION");
	int version;
	if (!(input >> version) || version != 1)
		throw IOError("Unsupported champion format");

	expect(input, "RECTANGLE");
	const float width = readFloat(input);
	const float height = readFloat(input);
	const float roadWidth = readFloat(input);
	Track track{width, height, roadWidth};

	expect(input, "OBSERVATIONS");
	int observationVersion;
	if (!(input >> observationVersion))
		throw IOError("Missing observation version");
	const float sensorRange = readFloat(input);
	const float maxSpeed = readFloat(input);
	if (observationVersion != 1 || sensorRange != 40.0f || maxSpeed != 50.0f)
		throw IOError("Incompatible observation configuration");

	expect(input, "CONTROLS");
	int controlsVersion;
	if (!(input >> controlsVersion) || controlsVersion != 1)
		throw IOError("Incompatible controls configuration");

	expect(input, "LAP_TIME");
	const float lapTime = readFloat(input);
	if (lapTime <= 0.0f)
		throw IOError("Invalid saved lap time");

	expect(input, "LAYERS");
	const std::size_t layerCount = readCount(input, 64);
	std::vector<Layer> layers;
	layers.reserve(layerCount);

	std::size_t expectedInputs = 8;
	std::size_t parameterCount = 0;

	for (std::size_t l = 0; l < layerCount; ++l) {
		expect(input, "NEURONS");
		const std::size_t neuronCount = readCount(input, 4096);

		std::vector<Neuron> neurons;
		neurons.reserve(neuronCount);

		for (std::size_t n = 0; n < neuronCount; ++n) {
			expect(input, "NEURON");

			const Activation activation = readActivation(input);
			const float bias = readFloat(input);
			const std::size_t inputCount = readCount(input, 4096);

			if (inputCount != expectedInputs)
				throw IOError("Incompatible saved layer dimensions");

			parameterCount += inputCount + 1;
			if (parameterCount > 1'000'000)
				throw IOError("Saved network is too large");

			std::vector<float> weights;
			weights.reserve(inputCount);

			for (std::size_t i = 0; i < inputCount; ++i)
				weights.push_back(readFloat(input));

			neurons.emplace_back(std::move(weights), bias, activation);
		}

		layers.emplace_back(std::move(neurons));
		expectedInputs = neuronCount;
	}

	if (expectedInputs != 3)
		throw IOError("A driving network must have three outputs");

	expect(input, "END");
	input >> std::ws;

	if (!input.eof())
		throw IOError("Unexpected data after champion");

	return SavedChampion{std::move(track), Network{std::move(layers)}, lapTime};
}
