/* ****************************************** */
/*   Track.hpp                  	          */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/10 22:41:11 by Luc      */
/*   Updated: 2026/10/10 22:41:11 by Luc      */
/* ****************************************** */

#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

#include "Vec2.hpp"
#include "Exception.hpp"

struct RoadSection {
	Vec2 center;
	Vec2 size;
};

class Track {
	public:
		Track(float width, float height, float roadWidth);

		// Accessors
		[[nodiscard]] const std::array<RoadSection, 4>& sections() const noexcept;
		[[nodiscard]] Vec2 startPosition() const noexcept;

		[[nodiscard]] bool canDrive(Vec2 position, float radius) const noexcept;
		[[nodiscard]] float raycast(Vec2 origin, float angle, float maxDistance) const;
		[[nodiscard]] float referenceHeading(Vec2 position) const;
		[[nodiscard]] float length() const noexcept;
		[[nodiscard]] float progressAt(Vec2 position) const;

	private:
		std::array<RoadSection, 4> _sections{};
		Vec2 _startPosition{};

		float _width;
		float _height;
		float _roadWidth;
};
