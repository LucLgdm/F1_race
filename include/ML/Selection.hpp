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

[[nodiscard]]
inline std::size_t selectBestIndex(std::span<const float> scores) {
	if (scores.empty())
		throw neuralError("Cannot select from an empty population");

	const auto best = std::max_element(scores.begin(), scores.end());

	return static_cast<std::size_t>(best - scores.begin());
}