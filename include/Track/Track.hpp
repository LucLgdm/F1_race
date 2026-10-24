#pragma once

#include <optional>
#include <string>
#include <vector>
#include "Vec2.hpp"
#include "Exception.hpp"

// Metres, radians, positive angles turn left. A rounded segment is a
// quarter annulus, tangent to its local +X axis at its start.
struct TrackSegment {
	Vec2 start{};
	float heading = 0.0f;
	float length = 80.0f;
	bool rounded = false;
	float radius = 30.0f;
	int turn = -1;
};

struct TrackLocation {
	std::size_t segment = 0;
	float localDistance = 0.0f;
	float coordinate = 0.0f;	  // wrapped distance from the starting line
	float lateralDistance = 0.0f; // signed along the local left normal
	float heading = 0.0f;
	Vec2 point{};
};
struct TrackEdge {
	Vec2 a, b;
};
struct TrackTriangle {
	Vec2 a, b, c;
};

class Track {
  public:
	Track(float width, float height, float roadWidth); // legacy rectangle
	Track(std::vector<TrackSegment> segments, float roadWidth, float startDistance = 0.0f);
	[[nodiscard]] const std::vector<TrackSegment>& segments() const noexcept { return _segments; }
	[[nodiscard]] const std::vector<TrackTriangle>& mesh() const noexcept { return _mesh; }
	[[nodiscard]] const std::vector<TrackEdge>& boundaries() const noexcept { return _boundaries; }
	[[nodiscard]] float roadWidth() const noexcept { return _roadWidth; }
	[[nodiscard]] float startDistance() const noexcept { return _startDistance; }
	[[nodiscard]] float length() const noexcept { return _length; }
	[[nodiscard]] Vec2 startPosition() const noexcept;
	[[nodiscard]] float startHeading() const noexcept;
	[[nodiscard]] Vec2 pointAt(float coordinate) const noexcept;
	[[nodiscard]] float headingAt(float coordinate) const noexcept;
	[[nodiscard]] TrackLocation locate(Vec2 position, std::optional<float> previous = {},
									   float maxTravel = 0.0f) const;
	[[nodiscard]] float progressAt(Vec2 position) const { return locate(position).coordinate; }
	[[nodiscard]] float referenceHeading(Vec2 position) const { return locate(position).heading; }
	[[nodiscard]] bool canDrive(Vec2 position, float radius) const noexcept;
	[[nodiscard]] bool canTravel(Vec2 from, Vec2 to, float radius) const noexcept;
	[[nodiscard]] float raycast(Vec2 origin, float angle, float maxDistance) const;
	[[nodiscard]] std::vector<std::string> issues() const;
	[[nodiscard]] std::vector<Vec2> polyline(std::size_t segment) const;
	[[nodiscard]] static float segmentLength(const TrackSegment& segment) noexcept;
	[[nodiscard]] static Vec2 segmentPoint(const TrackSegment& segment, float distance) noexcept;
	[[nodiscard]] static float segmentHeading(const TrackSegment& segment, float distance) noexcept;

  private:
	std::vector<TrackSegment> _segments;
	float _roadWidth;
	float _startDistance;
	float _length = 0.0f;
	std::vector<float> _offsets;
	std::vector<TrackTriangle> _mesh;
	std::vector<TrackEdge> _boundaries;
	void rebuild();
	[[nodiscard]] TrackLocation atRawDistance(float distance) const noexcept;
	[[nodiscard]] bool contains(Vec2 point) const noexcept;
};
