/* ****************************************** */
/*   Track.cpp                                */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/10 22:44:53 by Luc      */
/*   Updated: 2026/10/10 22:44:53 by Luc      */
/* ****************************************** */

#include "Track.hpp"

Track::Track(float width, float height, float roadWidth) : _width(width), _height(height), _roadWidth(roadWidth) {
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

/*********************************************************************
 * There is two borders, the outer rectangle and the inner rectangle.
 * The car must stay between these two borders.
 * The car is represented by a circle with a radius.
 *********************************************************************/

bool Track::canDrive(Vec2 position, float radius) const noexcept {
	const float outerHalfWidth = _width / 2.0f;
	const float outerHalfHeight = _height / 2.0f;

	// The boundary circle must stay within the outer rectangle
	if (std::abs(position.x) + radius > outerHalfWidth || std::abs(position.y) + radius > outerHalfHeight)
		return false;
	
	const float innerHalfWidth = outerHalfWidth - _roadWidth;
	const float innerHalfHeight = outerHalfHeight - _roadWidth;

	const float closestX = std::clamp(position.x, -innerHalfWidth, innerHalfWidth);
	const float closestY = std::clamp(position.y, -innerHalfHeight, innerHalfHeight);

	const float dx = position.x - closestX;
	const float dy = position.y - closestY;
	return (dx * dx + dy * dy) >= (radius * radius);
}
