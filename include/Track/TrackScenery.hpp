#pragma once

#include "Track/Track.hpp"
#include <array>
#include <vector>

struct SceneryVertex {
	Vec2 position;
	std::array<float, 3> color;
};

struct SceneryTree {
	Vec2 position;
	float radius;
	float shade;
	float rotation;
};

// Render-only geometry. The first vertices form triangles; the rest form lines.
struct TrackScenery {
	static constexpr float curbWidth = 0.8f;
	static constexpr float treeClearance = 2.5f;
	static constexpr float viewPadding = 26.0f;
	static constexpr std::size_t maxTrees = 320;
	std::vector<SceneryVertex> vertices;
	std::size_t triangleVertexCount = 0;
	std::vector<SceneryTree> trees;
};

[[nodiscard]] TrackScenery buildTrackScenery(const Track& track);
