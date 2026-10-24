#include "Track/Track.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace {
constexpr float pi = std::numbers::pi_v<float>;
constexpr float joinTolerance = 0.002f;
Vec2 add(Vec2 a, Vec2 b) {
	return {a.x + b.x, a.y + b.y};
}
Vec2 sub(Vec2 a, Vec2 b) {
	return {a.x - b.x, a.y - b.y};
}
Vec2 mul(Vec2 a, float k) {
	return {a.x * k, a.y * k};
}
float dot(Vec2 a, Vec2 b) {
	return a.x * b.x + a.y * b.y;
}
float cross(Vec2 a, Vec2 b) {
	return a.x * b.y - a.y * b.x;
}
float norm(Vec2 v) {
	return std::hypot(v.x, v.y);
}
Vec2 direction(float a) {
	return {std::cos(a), std::sin(a)};
}
Vec2 normal(float a) {
	return {-std::sin(a), std::cos(a)};
}
bool finite(Vec2 p) {
	return std::isfinite(p.x) && std::isfinite(p.y);
}
float wrap(float x, float length) {
	x = std::fmod(x, length);
	return x < 0 ? x + length : x;
}
float distanceSquared(Vec2 p, Vec2 a, Vec2 b) {
	const Vec2 d = sub(b, a);
	const float t = dot(d, d) > 0 ? std::clamp(dot(sub(p, a), d) / dot(d, d), 0.0f, 1.0f) : 0;
	const Vec2 v = sub(p, add(a, mul(d, t)));
	return dot(v, v);
}
bool intersects(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
	const Vec2 u = sub(b, a), v = sub(d, c);
	const float det = cross(u, v);
	if (std::abs(det) < 1e-6f)
		return false;
	const float t = cross(sub(c, a), v) / det, s = cross(sub(c, a), u) / det;
	return t > 1e-5f && t < 1 - 1e-5f && s > 1e-5f && s < 1 - 1e-5f;
}
float edgeDistanceSquared(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
	if (intersects(a, b, c, d))
		return 0;
	return std::min({distanceSquared(a, c, d), distanceSquared(b, c, d), distanceSquared(c, a, b),
					 distanceSquared(d, a, b)});
}
Vec2 joinedOffset(float incoming, float outgoing, float halfWidth) {
	const Vec2 n1 = normal(incoming), n2 = normal(outgoing), sum = add(n1, n2);
	const float divisor = dot(sum, n2);
	return divisor > 0.125f ? mul(sum, halfWidth / divisor) : mul(n2, halfWidth);
}
} // namespace

Track::Track(float width, float height, float roadWidth) : _roadWidth(roadWidth), _startDistance(0) {
	if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(roadWidth) || width <= 0 ||
		height <= 0 || roadWidth <= 0 || roadWidth >= std::min(width, height) / 2)
		throw trackError("Invalid rectangle dimensions");
	const float x = (width - roadWidth) / 2, y = (height - roadWidth) / 2;
	_segments = {
		{{-x, y}, 0, 2 * x}, {{x, y}, -pi / 2, 2 * y}, {{x, -y}, -pi, 2 * x}, {{-x, -y}, pi / 2, 2 * y}};
	_startDistance = x;
	rebuild();
}
Track::Track(std::vector<TrackSegment> segments, float roadWidth, float startDistance)
	: _segments(std::move(segments)), _roadWidth(roadWidth), _startDistance(startDistance) {
	rebuild();
}
float Track::segmentLength(const TrackSegment& s) noexcept {
	return s.rounded ? s.radius * pi / 2 : s.length;
}
float Track::segmentHeading(const TrackSegment& s, float distance) noexcept {
	return s.heading + (s.rounded ? s.turn * distance / s.radius : 0);
}
Vec2 Track::segmentPoint(const TrackSegment& s, float distance) noexcept {
	if (!s.rounded)
		return add(s.start, mul(direction(s.heading), distance));
	const float t = distance / s.radius;
	return add(s.start, add(mul(direction(s.heading), s.radius * std::sin(t)),
							mul(normal(s.heading), s.turn * s.radius * (1 - std::cos(t)))));
}
std::vector<Vec2> Track::polyline(std::size_t index) const {
	const auto& s = _segments.at(index);
	// Shared tessellation: at most 2 degrees or 0.75 m per arc chord.
	const int steps =
		s.rounded
			? std::clamp(static_cast<int>(std::ceil(std::max(45.0f, segmentLength(s) / 0.75f))), 45, 2048)
			: 1;
	std::vector<Vec2> points;
	points.reserve(steps + 1);
	for (int j = 0; j <= steps; ++j)
		points.push_back(segmentPoint(s, segmentLength(s) * j / steps));
	return points;
}
void Track::rebuild() {
	if (!std::isfinite(_roadWidth) || _roadWidth <= 0 || _roadWidth > 1000 ||
		!std::isfinite(_startDistance) || _segments.size() > 256)
		throw trackError("Invalid track dimensions or segment count");
	_length = 0;
	_offsets.clear();
	_mesh.clear();
	_boundaries.clear();
	for (const auto& s : _segments) {
		if (!finite(s.start) || std::abs(s.start.x) > 100000 || std::abs(s.start.y) > 100000 ||
			!std::isfinite(s.heading) || !std::isfinite(s.length) || !std::isfinite(s.radius) ||
			s.length < 0.5f || s.length > 10000 || s.radius <= 0 ||
			(s.rounded && s.radius <= _roadWidth / 2) || s.radius > 10000 || (s.turn != 1 && s.turn != -1))
			throw trackError("Invalid segment dimensions (curve radius must exceed half the road width)");
		_offsets.push_back(_length);
		_length += segmentLength(s);
	}
	if (_segments.empty()) {
		_startDistance = 0;
		return;
	}
	_startDistance = wrap(_startDistance, _length);
	for (std::size_t i = 0; i < _segments.size(); ++i) {
		const auto& s = _segments[i];
		const auto& prev = _segments[(i + _segments.size() - 1) % _segments.size()];
		const auto& next = _segments[(i + 1) % _segments.size()];
		const bool startJoined = norm(sub(s.start, segmentPoint(prev, segmentLength(prev)))) <= joinTolerance;
		const bool endJoined = norm(sub(segmentPoint(s, segmentLength(s)), next.start)) <= joinTolerance;
		auto points = polyline(i);
		// Canonical shared vertices also close sub-millimetre rounding gaps
		// after a text export/import, while retaining the analytical centerline.
		if (startJoined)
			points.front() = mul(add(s.start, segmentPoint(prev, segmentLength(prev))), 0.5f);
		if (endJoined)
			points.back() = mul(add(segmentPoint(s, segmentLength(s)), next.start), 0.5f);
		std::vector<Vec2> left, right;
		if (_mesh.size() + 2 * (points.size() - 1) > 8192)
			throw trackError("Track tessellation is too large");
		for (std::size_t j = 0; j < points.size(); ++j) {
			const float d = segmentLength(s) * j / (points.size() - 1);
			Vec2 offset = mul(normal(segmentHeading(s, d)), _roadWidth / 2);
			if (j == 0 && startJoined)
				offset = joinedOffset(segmentHeading(prev, segmentLength(prev)), s.heading, _roadWidth / 2);
			if (j + 1 == points.size() && endJoined)
				offset = joinedOffset(segmentHeading(s, segmentLength(s)), next.heading, _roadWidth / 2);
			left.push_back(add(points[j], offset));
			right.push_back(sub(points[j], offset));
		}
		for (std::size_t j = 1; j < points.size(); ++j) {
			_mesh.push_back({left[j - 1], right[j - 1], right[j]});
			_mesh.push_back({left[j - 1], right[j], left[j]});
			_boundaries.push_back({left[j - 1], left[j]});
			_boundaries.push_back({right[j - 1], right[j]});
		}
		if (!startJoined)
			_boundaries.push_back({left.front(), right.front()});
		if (!endJoined)
			_boundaries.push_back({left.back(), right.back()});
	}
}
TrackLocation Track::atRawDistance(float distance) const noexcept {
	TrackLocation result;
	if (_segments.empty())
		return result;
	distance = wrap(distance, _length);
	const auto it = std::upper_bound(_offsets.begin(), _offsets.end(), distance);
	result.segment = static_cast<std::size_t>(it - _offsets.begin() - 1);
	result.localDistance = distance - _offsets[result.segment];
	result.coordinate = wrap(distance - _startDistance, _length);
	result.point = segmentPoint(_segments[result.segment], result.localDistance);
	result.heading = segmentHeading(_segments[result.segment], result.localDistance);
	return result;
}
Vec2 Track::startPosition() const noexcept {
	return atRawDistance(_startDistance).point;
}
float Track::startHeading() const noexcept {
	return atRawDistance(_startDistance).heading;
}
Vec2 Track::pointAt(float coordinate) const noexcept {
	return atRawDistance(coordinate + _startDistance).point;
}
float Track::headingAt(float coordinate) const noexcept {
	return atRawDistance(coordinate + _startDistance).heading;
}
TrackLocation Track::locate(Vec2 position, std::optional<float> previous, float maxTravel) const {
	if (!finite(position) || _segments.empty() ||
		(previous && (!std::isfinite(*previous) || !std::isfinite(maxTravel) || maxTravel <= 0)))
		throw trackError("Invalid track projection");
	float best = std::numeric_limits<float>::infinity();
	TrackLocation result;
	for (std::size_t i = 0; i < _segments.size(); ++i) {
		const auto& s = _segments[i];
		float distance = 0;
		if (!s.rounded)
			distance = dot(sub(position, s.start), direction(s.heading));
		else {
			const Vec2 center = add(s.start, mul(normal(s.heading), s.turn * s.radius));
			const Vec2 v = sub(position, center);
			const float a = std::atan2(dot(v, direction(s.heading)), -s.turn * dot(v, normal(s.heading)));
			if (a >= 0 && a <= pi / 2)
				distance = a * s.radius;
			else
				distance =
					norm(sub(position, s.start)) <= norm(sub(position, segmentPoint(s, segmentLength(s))))
						? 0
						: segmentLength(s);
		}
		distance = std::clamp(distance, 0.0f, segmentLength(s));
		const float coordinate = wrap(_offsets[i] + distance - _startDistance, _length);
		if (previous && std::abs(std::remainder(coordinate - *previous, _length)) > maxTravel)
			continue;
		const Vec2 point = segmentPoint(s, distance), offset = sub(position, point);
		const float score = dot(offset, offset);
		if (score < best) {
			best = score;
			result = {i,
					  distance,
					  coordinate,
					  dot(offset, normal(segmentHeading(s, distance))),
					  segmentHeading(s, distance),
					  point};
		}
	}
	// Keep the local coordinate instead of jumping to an unrelated nearby lane.
	if (!std::isfinite(best)) {
		result = atRawDistance(*previous + _startDistance);
		result.lateralDistance = dot(sub(position, result.point), normal(result.heading));
	}
	return result;
}
bool Track::contains(Vec2 p) const noexcept {
	for (const auto& t : _mesh) {
		if (std::abs(cross(sub(t.b, t.a), sub(t.c, t.a))) < 1e-6f)
			continue;
		const float a = cross(sub(t.b, t.a), sub(p, t.a)), b = cross(sub(t.c, t.b), sub(p, t.b)),
					c = cross(sub(t.a, t.c), sub(p, t.c));
		if ((a >= -1e-4f && b >= -1e-4f && c >= -1e-4f) || (a <= 1e-4f && b <= 1e-4f && c <= 1e-4f))
			return true;
	}
	return false;
}
bool Track::canDrive(Vec2 p, float radius) const noexcept {
	if (!finite(p) || !std::isfinite(radius) || radius < 0 || !contains(p))
		return false;
	for (const auto& edge : _boundaries)
		if (distanceSquared(p, edge.a, edge.b) + 1e-5f < radius * radius)
			return false;
	return true;
}
bool Track::canTravel(Vec2 from, Vec2 to, float radius) const noexcept {
	if (!canDrive(from, radius) || !canDrive(to, radius))
		return false;
	for (const auto& edge : _boundaries)
		if (intersects(from, to, edge.a, edge.b) ||
			edgeDistanceSquared(from, to, edge.a, edge.b) + 1e-5f < radius * radius)
			return false;
	return true;
}
float Track::raycast(Vec2 origin, float angle, float maxDistance) const {
	if (!finite(origin) || !std::isfinite(angle) || !std::isfinite(maxDistance) || maxDistance <= 0)
		throw trackError("Invalid raycast argument");
	if (!contains(origin))
		return 0;
	const Vec2 d = direction(angle);
	float result = maxDistance;
	for (const auto& edge : _boundaries) {
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
std::vector<std::string> Track::issues() const {
	std::vector<std::string> result;
	if (_segments.size() < 2)
		result.push_back("Ajoute des segments pour former une boucle.");
	for (std::size_t i = 0; i < _segments.size(); ++i) {
		const auto& s = _segments[i];
		const auto& next = _segments[(i + 1) % _segments.size()];
		if (norm(sub(segmentPoint(s, segmentLength(s)), next.start)) > joinTolerance) {
			result.push_back("Les extremites ne forment pas une boucle fermee.");
			break;
		}
		if (std::cos(segmentHeading(s, segmentLength(s)) - next.heading) < -0.875f) {
			result.push_back("Un raccord forme un demi-tour trop serre.");
			break;
		}
	}
	bool crossing = false;
	std::vector<TrackEdge> centerline;
	for (std::size_t i = 0; i < _segments.size(); ++i) {
		const auto points = polyline(i);
		for (std::size_t j = 1; j < points.size(); ++j)
			centerline.push_back({points[j - 1], points[j]});
	}
	for (std::size_t i = 0; i < centerline.size() && !crossing; ++i) {
		for (std::size_t j = i + 2; j < centerline.size(); ++j) {
			if (i == 0 && j + 1 == centerline.size())
				continue;
			const auto& a = centerline[i];
			const auto& b = centerline[j];
			if (std::max(a.a.x, a.b.x) + joinTolerance < std::min(b.a.x, b.b.x) ||
				std::max(b.a.x, b.b.x) + joinTolerance < std::min(a.a.x, a.b.x))
				continue;
			if (edgeDistanceSquared(a.a, a.b, b.a, b.b) <= joinTolerance * joinTolerance) {
				crossing = true;
				break;
			}
		}
	}
	for (std::size_t i = 0; i < _boundaries.size() && !crossing; ++i)
		for (std::size_t j = i + 1; j < _boundaries.size(); ++j)
			if (std::max(_boundaries[i].a.x, _boundaries[i].b.x) >=
					std::min(_boundaries[j].a.x, _boundaries[j].b.x) &&
				std::max(_boundaries[j].a.x, _boundaries[j].b.x) >=
					std::min(_boundaries[i].a.x, _boundaries[i].b.x) &&
				intersects(_boundaries[i].a, _boundaries[i].b, _boundaries[j].a, _boundaries[j].b)) {
				crossing = true;
				break;
			}
	if (crossing)
		result.push_back("Des bords de piste se croisent : espace les pieces ou reduis la largeur.");
	if (_segments.size() > 1 && !canDrive(startPosition(), 2.3f))
		result.push_back("Le depart ne laisse pas assez de place pour la voiture.");
	return result;
}
