#include "Track/TrackScenery.hpp"
#include "Track/Bvh2D.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <random>

namespace {
using Color = std::array<float, 3>;
constexpr Color asphalt{0.28f, 0.30f, 0.29f};
constexpr Color curbWhite{0.94f, 0.94f, 0.90f};
constexpr Color curbBlack{0.035f, 0.04f, 0.035f};

Vec2 add(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
Vec2 sub(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
Vec2 mul(Vec2 a, float s) { return {a.x * s, a.y * s}; }
float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
Vec2 lerp(Vec2 a, Vec2 b, float t) {
	if (t <= 0) return a;
	if (t >= 1) return b;
	return add(a, mul(sub(b, a), t));
}

void triangle(TrackScenery& scene, Vec2 a, Vec2 b, Vec2 c, Color color) {
	const auto ab = sub(b, a), ac = sub(c, a);
	if (ab.x * ac.y - ab.y * ac.x < 0)
		std::swap(b, c);
	scene.vertices.insert(scene.vertices.end(), {{a, color}, {b, color}, {c, color}});
}
void quad(TrackScenery& scene, Vec2 a, Vec2 b, Vec2 c, Vec2 d, Color color) {
	triangle(scene, a, b, c, color);
	triangle(scene, a, c, d, color);
}
void ellipse(TrackScenery& scene, Vec2 center, float rx, float ry, Color color, float rotation = 0) {
	constexpr int sides = 16;
	const float cs = std::cos(rotation), sn = std::sin(rotation);
	const auto point = [&](int i) {
		const float a = i * 2 * std::numbers::pi_v<float> / sides;
		const float x = std::cos(a) * rx, y = std::sin(a) * ry;
		return Vec2{center.x + x * cs - y * sn, center.y + x * sn + y * cs};
	};
	for (int i = 0; i < sides; ++i)
		triangle(scene, center, point(i), point(i + 1), color);
}

void curbs(TrackScenery& scene, const Track& track) {
	// Track::rebuild emits two triangles for every ribbon quad. Extruding its
	// shared cross-sections preserves the exact miters and curved joins.
	const auto& mesh = track.mesh();
	const double scale = static_cast<double>(TrackScenery::curbWidth) / track.roadWidth();
	const auto curbOffset = [scale](Vec2 left, Vec2 right) {
		return Vec2{static_cast<float>((double(left.x) - right.x) * scale),
					static_cast<float>((double(left.y) - right.y) * scale)};
	};
	const double stripeLength = std::max(2.0, static_cast<double>(track.length()) / 20000.0);
	double travelled = 0;
	for (std::size_t i = 0; i + 1 < mesh.size(); i += 2) {
		const Vec2 left0 = mesh[i].a, right0 = mesh[i].b;
		const Vec2 right1 = mesh[i].c, left1 = mesh[i + 1].c;
		const Vec2 center0 = mul(add(left0, right0), 0.5f);
		const Vec2 center1 = mul(add(left1, right1), 0.5f);
		const double length = std::hypot(center1.x - center0.x, center1.y - center0.y);
		if (length < 1e-6)
			continue;
		const Vec2 offset0 = curbOffset(left0, right0);
		const Vec2 offset1 = curbOffset(left1, right1);
		const double end = travelled + length;
		double start = travelled;
		std::size_t band = static_cast<std::size_t>(std::floor(start / stripeLength));
		while (start < end) {
			const double next = std::min(end, (band + 1) * stripeLength);
			if (next <= start) {
				++band;
				continue;
			}
			const float t0 = static_cast<float>((start - travelled) / length);
			const float t1 = static_cast<float>((next - travelled) / length);
			const auto color = band % 2 == 0 ? curbWhite : curbBlack;
			quad(scene, lerp(left0, left1, t0), lerp(left0, left1, t1),
				 lerp(add(left0, offset0), add(left1, offset1), t1),
				 lerp(add(left0, offset0), add(left1, offset1), t0), color);
			quad(scene, lerp(right0, right1, t0), lerp(sub(right0, offset0), sub(right1, offset1), t0),
				 lerp(sub(right0, offset0), sub(right1, offset1), t1), lerp(right0, right1, t1), color);
			start = next;
			++band;
		}
		travelled = end;
	}
}

float edgeDistanceSquared(Vec2 point, const TrackEdge& edge) {
	const Vec2 delta = sub(edge.b, edge.a);
	const float length = dot(delta, delta);
	const float t = length > 0 ? std::clamp(dot(sub(point, edge.a), delta) / length, 0.0f, 1.0f) : 0;
	const Vec2 offset = sub(point, add(edge.a, mul(delta, t)));
	return dot(offset, offset);
}
std::uint32_t scenerySeed(const Track& track) {
	std::uint32_t hash = 2166136261u;
	const auto mix = [&](float value) {
		hash = (hash ^ std::bit_cast<std::uint32_t>(value == 0 ? 0.0f : value)) * 16777619u;
	};
	mix(track.roadWidth());
	for (const auto& segment : track.segments()) {
		mix(segment.start.x); mix(segment.start.y); mix(segment.heading);
		mix(segment.rounded ? segment.radius : segment.length);
		mix(segment.rounded ? static_cast<float>(segment.turn) : 0.0f);
	}
	return hash;
}

void placeTrees(TrackScenery& scene, const Track& track) {
	std::vector<Aabb2D> boxes;
	boxes.reserve(track.boundaries().size());
	for (const auto& edge : track.boundaries()) {
		Aabb2D box;
		box.expand(edge.a);
		box.expand(edge.b);
		boxes.push_back(box);
	}
	Bvh2D index;
	index.build(boxes);
	std::vector<float> segmentStarts;
	float total = 0;
	for (const auto& segment : track.segments()) {
		segmentStarts.push_back(total);
		total += Track::segmentLength(segment);
	}
	std::mt19937 generator{scenerySeed(track)};
	std::uniform_real_distribution<float> unit{0, 1};
	const int stations = std::clamp(static_cast<int>(std::ceil(track.length() / 22)), 8, 512);
	const float spacing = track.length() / stations;
	for (int i = 0; i < stations && scene.trees.size() < TrackScenery::maxTrees; ++i) {
		for (float side : {-1.0f, 1.0f}) {
			if (scene.trees.size() >= TrackScenery::maxTrees)
				break;
			if (unit(generator) > 0.72f)
				continue;
			const float radius = 1.5f + unit(generator) * 1.2f;
			const float clearance = radius + TrackScenery::curbWidth + TrackScenery::treeClearance;
			// Anchor planting to the first piece, independently of the starting line.
			const float coordinate = (i + 0.5f + (unit(generator) - 0.5f) * 0.65f) * spacing;
			const auto found = std::upper_bound(segmentStarts.begin(), segmentStarts.end(), coordinate);
			const auto segmentIndex = static_cast<std::size_t>(found - segmentStarts.begin() - 1);
			const auto& segment = track.segments()[segmentIndex];
			const float localDistance = coordinate - segmentStarts[segmentIndex];
			const Vec2 center = Track::segmentPoint(segment, localDistance);
			const float heading = Track::segmentHeading(segment, localDistance);
			const float offset = side * (track.roadWidth() / 2 + clearance + unit(generator) * 11);
			const Vec2 position{center.x - std::sin(heading) * offset, center.y + std::cos(heading) * offset};
			if (track.canDrive(position, 0))
				continue;
			const Aabb2D query{{position.x - clearance, position.y - clearance},
							 {position.x + clearance, position.y + clearance}};
			if (index.anyOverlapping(query, [&](std::size_t edge) {
				return edgeDistanceSquared(position, track.boundaries()[edge]) < clearance * clearance;
			}))
				continue;
			if (std::any_of(scene.trees.begin(), scene.trees.end(), [&](const auto& other) {
				const Vec2 delta = sub(position, other.position);
				const float separation = radius + other.radius + 2;
				return dot(delta, delta) < separation * separation;
			}))
				continue;
			scene.trees.push_back({position, radius, (unit(generator) - 0.5f) * 0.05f,
				unit(generator) * 2 * std::numbers::pi_v<float>});
		}
	}
}

void drawTrees(TrackScenery& scene) {
	for (const auto& tree : scene.trees)
		ellipse(scene, add(tree.position, {0.7f, -0.8f}), tree.radius * 1.12f, tree.radius * 0.86f,
				{0.055f, 0.085f, 0.05f}, -0.25f);
	for (const auto& tree : scene.trees) {
		const auto p = tree.position;
		const float r = tree.radius, shade = tree.shade;
		ellipse(scene, add(p, {0, -r * 0.82f}), r * 0.17f, r * 0.28f, {0.24f, 0.14f, 0.065f});
		ellipse(scene, p, r, r * 0.93f, {0.055f, 0.20f + shade, 0.07f}, tree.rotation);
		ellipse(scene, add(p, {-r * 0.13f, r * 0.14f}), r * 0.81f, r * 0.75f,
				{0.085f, 0.29f + shade, 0.09f}, tree.rotation);
		for (int lobe = 0; lobe < 3; ++lobe) {
			const float angle = tree.rotation + lobe * 2.1f;
			const Vec2 center{p.x + r * (std::cos(angle) * 0.27f - 0.12f),
							  p.y + r * (std::sin(angle) * 0.25f + 0.16f)};
			ellipse(scene, center, r * 0.42f, r * 0.37f,
					{0.12f, 0.35f + shade + lobe * 0.012f, 0.115f}, angle);
		}
	}
}
} // namespace

TrackScenery buildTrackScenery(const Track& track) {
	TrackScenery scene;
	if (track.mesh().empty())
		return scene;
	curbs(scene, track);
	for (const auto& face : track.mesh())
		triangle(scene, face.a, face.b, face.c, asphalt);
	placeTrees(scene, track);
	drawTrees(scene);
	scene.triangleVertexCount = scene.vertices.size();
	for (const auto& edge : track.boundaries()) {
		scene.vertices.push_back({edge.a, {0.85f, 0.88f, 0.82f}});
		scene.vertices.push_back({edge.b, {0.85f, 0.88f, 0.82f}});
	}
	const Vec2 p = track.startPosition();
	const float heading = track.startHeading(), halfWidth = track.roadWidth() / 2;
	scene.vertices.push_back({{p.x - std::sin(heading) * halfWidth, p.y + std::cos(heading) * halfWidth}, {1, 0.7f, 0.3f}});
	scene.vertices.push_back({{p.x + std::sin(heading) * halfWidth, p.y - std::cos(heading) * halfWidth}, {1, 0.7f, 0.3f}});
	return scene;
}
