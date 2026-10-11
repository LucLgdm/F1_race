/* ****************************************** */
/*   Loss.hpp                                 */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 22:46:48 by Luc      */
/*   Updated: 2026/10/11 22:46:48 by Luc      */
/* ****************************************** */

#pragma once

#include <span>

#include "Exception.hpp"

inline float meanSquaredError(std::span<const float> predictions, std::span<const float> targets) {
	if (predictions.empty() || predictions.size() != targets.size())
		throw neuralError("Predictions and targets must have the same non-zero size.");
	
	// Compute the mean squared error
	float total = 0.0f;
	for(std::size_t i = 0; i < predictions.size(); i++) {
		float diff = predictions[i] - targets[i];
		total += diff * diff;
	}
	return total / static_cast<float>(predictions.size());
}