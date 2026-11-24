#pragma once

#include "Track/Track.hpp"
#include <algorithm>
#include <cmath>

// Exhaustive queries from before spatial indexing, kept as an independent
// regression oracle. Only the public mesh and boundaries are consulted.
namespace track_reference {
inline Vec2 add(Vec2 a, Vec2 b) {
	return {a.x + b.x, a.y + b.y};
}
inline Vec2 sub(Vec2 a, Vec2 b) {
	return {a.x - b.x, a.y - b.y};
}
inline Vec2 mul(Vec2 a, float k) {
	return {a.x * k, a.y * k};
}
inline float dot(Vec2 a, Vec2 b) {
	return a.x * b.x + a.y * b.y;
}
inline float cross(Vec2 a, Vec2 b) {
	return a.x * b.y - a.y * b.x;
}
inline Vec2 direction(float a) {
	return {std::cos(a), std::sin(a)};
}
inline bool finite(Vec2 p) {
	return std::isfinite(p.x) && std::isfinite(p.y);
}
inline float distanceSquared(Vec2 p, Vec2 a, Vec2 b) {
	const Vec2 d = sub(b, a);
	const float t = dot(d, d) > 0 ? std::clamp(dot(sub(p, a), d) / dot(d, d), 0.0f, 1.0f) : 0;
	const Vec2 v = sub(p, add(a, mul(d, t)));
	return dot(v, v);
}
inline bool intersects(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
	const Vec2 u = sub(b, a), v = sub(d, c);
	const float det = cross(u, v);
	if (std::abs(det) < 1e-6f)
		return false;
	const float t = cross(sub(c, a), v) / det, s = cross(sub(c, a), u) / det;
	return t > 1e-5f && t < 1 - 1e-5f && s > 1e-5f && s < 1 - 1e-5f;
}
inline float edgeDistanceSquared(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
	if (intersects(a, b, c, d))
		return 0;
	return std::min({distanceSquared(a, c, d), distanceSquared(b, c, d), distanceSquared(c, a, b),
					 distanceSquared(d, a, b)});
}
inline bool contains(const Track& track, Vec2 p) {
	for (const auto& t : track.mesh()) {
		if (std::abs(cross(sub(t.b, t.a), sub(t.c, t.a))) < 1e-6f)
			continue;
		const float a = cross(sub(t.b, t.a), sub(p, t.a)), b = cross(sub(t.c, t.b), sub(p, t.b)),
					c = cross(sub(t.a, t.c), sub(p, t.c));
		if ((a >= -1e-4f && b >= -1e-4f && c >= -1e-4f) || (a <= 1e-4f && b <= 1e-4f && c <= 1e-4f))
			return true;
	}
	return false;
}
inline bool canDrive(const Track& track, Vec2 p, float radius) {
	if (!finite(p) || !std::isfinite(radius) || radius < 0 || !contains(track, p))
		return false;
	for (const auto& edge : track.boundaries())
		if (distanceSquared(p, edge.a, edge.b) + 1e-5f < radius * radius)
			return false;
	return true;
}
inline bool canTravel(const Track& track, Vec2 from, Vec2 to, float radius) {
	if (!canDrive(track, from, radius) || !canDrive(track, to, radius))
		return false;
	for (const auto& edge : track.boundaries())
		if (intersects(from, to, edge.a, edge.b) ||
			edgeDistanceSquared(from, to, edge.a, edge.b) + 1e-5f < radius * radius)
			return false;
	return true;
}
inline float raycast(const Track& track, Vec2 origin, float angle, float maxDistance) {
	if (!finite(origin) || !std::isfinite(angle) || !std::isfinite(maxDistance) || maxDistance <= 0)
		throw trackError("Invalid raycast argument");
	if (!contains(track, origin))
		return 0;
	const Vec2 d = direction(angle);
	float result = maxDistance;
	for (const auto& edge : track.boundaries()) {
		const Vec2 v = sub(edge.b, edge.a);
		const float det = cross(d, v);
		if (std::abs(det) < 1e-7f)
			continue;
		const Vec2 offset = sub(edge.a, origin);
		const float t = cross(offset, v) / det, u = cross(offset, d) / det;
		if (t >= -1e-4f && u >= -1e-5f && u <= 1.00001f)
			result = std::min(result, std::max(0.0f, t));
	}
	return result;
}
} // namespace track_reference
