#pragma once

#include "Track/Track.hpp"
#include <algorithm>
#include <limits>

// The orthographic camera needs a centre and a visible half-height.
struct CameraView {
	Vec2 position{};
	float halfHeight = 30.0f;
};

[[nodiscard]] inline CameraView fitTrackView(const Track& track, int viewportWidth, int viewportHeight) {
	if (viewportWidth <= 0 || viewportHeight <= 0)
		throw trackError("Camera viewport dimensions must be positive");
	if (track.boundaries().empty())
		return {};

	Vec2 minimum{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
	Vec2 maximum{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
	for (const auto& edge : track.boundaries()) {
		for (const Vec2 point : {edge.a, edge.b}) {
			minimum.x = std::min(minimum.x, point.x);
			minimum.y = std::min(minimum.y, point.y);
			maximum.x = std::max(maximum.x, point.x);
			maximum.y = std::max(maximum.y, point.y);
		}
	}

	constexpr float padding = 5.0f;
	const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
	const float halfWidth = (maximum.x - minimum.x) / 2.0f + padding;
	const float halfHeight = (maximum.y - minimum.y) / 2.0f + padding;
	return {{(minimum.x + maximum.x) / 2.0f, (minimum.y + maximum.y) / 2.0f},
			std::max(halfHeight, halfWidth / aspect) * 1.05f};
}
