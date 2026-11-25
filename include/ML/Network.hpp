/* ****************************************** */
/*   Network.hpp                              */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 21:55:40 by Luc      */
/*   Updated: 2026/10/11 21:55:40 by Luc      */
/* ****************************************** */

#pragma once

#include <span>
#include <vector>
#include <utility>

#include "ML/Layer.hpp"

class Network {
	public:
		explicit Network(std::vector<Layer> layers);

		[[nodiscard]] std::vector<float> forward(std::span<const float> inputs) const;
        [[nodiscard]] std::vector<std::size_t> topology() const;
        [[nodiscard]] Network expandedDrivingNetwork() const;
        // Contiguous layer-major weights + biases for future CPU SIMD / GPU upload.
        [[nodiscard]] std::vector<float> parameters() const;
        // Allocation-free inference; workspace belongs to the caller, never shared.
        void forwardInto(std::span<const float> inputs, std::span<float> output,
                         std::span<float> workspaceA, std::span<float> workspaceB) const;
        float train(std::span<const float> inputs, std::span<const float> targets, float learningRate);
		void mutate(float probability, float strength, std::mt19937& generator);

	private:
		std::vector<Layer> _layers;
	friend class ChampionIO;
};
