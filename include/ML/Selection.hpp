/* ****************************************** */
/*   Selection.hpp                            */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/19 16:52:40 by Luc      */
/*   Updated: 2026/10/19 16:52:40 by Luc      */
/* ****************************************** */

#pragma once

#include <algorithm>
#include <cstddef>
#include <span>

#include "Exception.hpp"

struct AgentScore {
	bool completedLap = false;
	float lapTime = 0.0f;
	float progress = 0.0f;
};

[[nodiscard]]
inline std::size_t selectBestIndex(std::span<const float> scores) {
	if (scores.empty())
		throw neuralError("Cannot select from an empty population");

	const auto best = std::max_element(scores.begin(), scores.end());

	return static_cast<std::size_t>(best - scores.begin());
}

[[nodiscard]]
inline std::size_t selectBestIndex(std::span<const AgentScore> scores) {
	if (scores.empty())
		throw neuralError("Cannot select from an empty population");

	std::size_t bestIndex = 0;

	for (std::size_t i = 0; i < scores.size(); ++i) {
		const AgentScore& candidate = scores[i];
		const AgentScore& best = scores[bestIndex];

		bool better = false;

		if (candidate.completedLap != best.completedLap) {
			better = candidate.completedLap;
		}else if (candidate.completedLap) {
			better = candidate.lapTime < best.lapTime;
		}else{
			better = candidate.progress > best.progress;
		}

		if (better)
			bestIndex = i;
	}
	return bestIndex;
}
