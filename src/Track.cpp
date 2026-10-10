/* ****************************************** */
/*   Track.cpp                                */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/10 22:44:53 by Luc      */
/*   Updated: 2026/10/10 22:44:53 by Luc      */
/* ****************************************** */

#include "Track.hpp"

Track::Track(float width, float height, float roadWidth) {
	if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(roadWidth)
		|| width <= 0.0f || height <= 0.0f || roadWidth <= 0.0f || roadWidth >= std::min(width, height) / 2.0f)
		throw trackError("Track dimensions must be finite numbers.");

	// Positions for a simple rectangular track
	const float horizontalY = (height - roadWidth) / 2.0f;
	const float verticalX = (width - roadWidth) / 2.0f;
	const float verticalLength = height - 2.0f * roadWidth;

	_sections = {	
		// Horizontal sections
		RoadSection{{0.0f,  horizontalY}, {width, roadWidth}},
		RoadSection{{0.0f, -horizontalY}, {width, roadWidth}},

		// Vertical sections, between the horizontal sections
		RoadSection{{-verticalX, 0.0f}, {roadWidth, verticalLength}},
		RoadSection{{ verticalX, 0.0f}, {roadWidth, verticalLength}}
	};
	_startPosition = {0.0f, horizontalY};
}

const std::array<RoadSection, 4>& Track::sections() const noexcept {
	return _sections;
}

Vec2 Track::startPosition() const noexcept {
	return _startPosition;
}

