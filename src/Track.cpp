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

float Track::raycast(Vec2 origin, float angle, float maxDistance) const {
	if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(angle)
		|| !std::isfinite(maxDistance) || maxDistance <= 0.0f)
		throw trackError("Invalid raycast argument");
	
	const Vec2 direction{std::cos(angle), std::sin(angle)};
	float closestDistance = maxDistance;

	// Intersection with a vertical wall
	const auto testVertical = [&](float x, float minY, float maxY) {
		if (std::abs(direction.x) < 1e-6f)
			return;
		
		const float distance = (x - origin.x) / direction.x;
		if (distance < 0.0f || distance > closestDistance)
			return;
		
		const float y = origin.y + distance * direction.y;
		if (y >= minY && y <= maxY)
			closestDistance = distance;
	};

	// Intresection with a horizontal wall
	const auto testHorizontal = [&](float y, float minX, float maxX) {
		if (std::abs(direction.y) < 1e-6f)
			return;

		const float distance = (y - origin.y) / direction.y;
		if (distance < 0.0f || distance > closestDistance)
			return;

		const float x = origin.x + distance * direction.x;
		if (x >= minX && x <= maxX)
			closestDistance = distance;
	};

	const auto testRectangle = [&](float halfWidth, float halfHeight) {
		testVertical(-halfWidth, -halfHeight, halfHeight);
		testVertical( halfWidth, -halfHeight, halfHeight);

		testHorizontal(-halfHeight, -halfWidth, halfWidth);
		testHorizontal( halfHeight, -halfWidth, halfWidth);
	};

	// 4 external wall
	testRectangle(_width / 2.0f, _height / 2.0f);

	// 4 wall around the central zone
	testRectangle(_width / 2.0f - _roadWidth, _height / 2.0f - _roadWidth);

	return closestDistance;
}

float Track::referenceHeading(Vec2 position) const {
	const float halfWidth = (_width - _roadWidth) / 2.0f;
	const float halfHeight = (_height - _roadWidth) / 2.0f;

	// Central line in clockwise
	const std::array<Vec2, 4> corners{{
		{-halfWidth,  halfHeight},
		{ halfWidth,  halfHeight},
		{ halfWidth, -halfHeight},
		{-halfWidth, -halfHeight}
	}};
	float closestSquaredDistance = std::numeric_limits<float>::max();
	float heading = 0.0f;

	for (std::size_t i = 0; i < corners.size(); ++i) {
		const Vec2 start = corners[i];
		const Vec2 end = corners[(i + 1) % corners.size()];

		const float dx = end.x - start.x;
		const float dy = end.y - start.y;
		const float squaredLength = dx * dx + dy * dy;

		// Projection of the position on the segment
		const float t = std::clamp(((position.x - start.x) * dx + (position.y - start.y) * dy) / squaredLength,
									0.0f, 1.0f);
		const float closestX = start.x + t * dx;
		const float closestY = start.y + t * dy;

		const float offsetX = position.x - closestX;
		const float offsetY = position.y - closestY;
		const float squaredDistance = offsetX * offsetX + offsetY * offsetY;

		if (squaredDistance < closestSquaredDistance) {
			closestSquaredDistance = squaredDistance;
			heading = std::atan2(dy, dx);
		}
	}
	return heading;
}