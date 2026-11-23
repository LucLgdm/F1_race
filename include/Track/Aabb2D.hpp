#pragma once

#include <algorithm>
#include <limits>
#include "Vec2.hpp"

// Axis-aligned bounds for finite 2D points. Default bounds are empty.
struct Aabb2D {
	Vec2 min{
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity()
	};
	Vec2 max{
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity()
	};

	void expand(Vec2 point) noexcept {
		min.x = std::min(min.x, point.x);
		min.y = std::min(min.y, point.y);
		max.x = std::max(max.x, point.x);
		max.y = std::max(max.y, point.y);
	}

	[[nodiscard]] bool contains(Vec2 point) const noexcept {
		return point.x >= min.x && point.x <= max.x
			&& point.y >= min.y && point.y <= max.y;
	}

	// Touching edges and corners count as an overlap.
	[[nodiscard]] bool overlaps(const Aabb2D& other) const noexcept {
		return min.x <= other.max.x && max.x >= other.min.x
			&& min.y <= other.max.y && max.y >= other.min.y;
	}
};
