#include "Track/TrackScenery.hpp"
#include "Track/ChampionIO.hpp"
#include "Track/TrackIO.hpp"
#include <fstream>
#include "CameraView.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <sstream>

namespace {
using Color = std::array<float, 3>;
constexpr Color white{0.94f, 0.94f, 0.90f};
constexpr Color black{0.035f, 0.04f, 0.035f};

std::optional<Color> colorAt(const TrackScenery& scene, Vec2 p) {
	std::optional<Color> result;
	const auto cross = [](Vec2 a, Vec2 b, Vec2 c) {
		return double(b.x - a.x) * (c.y - a.y) - double(b.y - a.y) * (c.x - a.x);
	};
	for (std::size_t i = 0; i < scene.triangleVertexCount; i += 3) {
		const Vec2 a = scene.vertices[i].position, b = scene.vertices[i + 1].position,
				   c = scene.vertices[i + 2].position;
		if (std::abs(cross(a, b, c)) > 1e-8 && cross(a, b, p) >= -1e-7 &&
			cross(b, c, p) >= -1e-7 && cross(c, a, p) >= -1e-7)
			result = scene.vertices[i].color;
	}
	return result;
}

double distanceToEdge(Vec2 p, const TrackEdge& edge) {
	const double dx = double(edge.b.x) - edge.a.x, dy = double(edge.b.y) - edge.a.y;
	const double length = dx * dx + dy * dy;
	const double t = length > 0 ? std::clamp(((p.x - edge.a.x) * dx + (p.y - edge.a.y) * dy) / length, 0.0, 1.0) : 0;
	return std::hypot(p.x - edge.a.x - t * dx, p.y - edge.a.y - t * dy);
}

bool sameTrees(const TrackScenery& a, const TrackScenery& b) {
	return a.trees.size() == b.trees.size() && std::equal(a.trees.begin(), a.trees.end(), b.trees.begin(),
		[](const auto& x, const auto& y) {
			return x.position.x == y.position.x && x.position.y == y.position.y &&
				x.radius == y.radius && x.shade == y.shade && x.rotation == y.rotation;
		});
}
} // namespace

TEST_CASE("Les bandes noires et blanches alternent sur les deux bords", "[scenery]") {
	const Track track{{TrackSegment{{0, 0}, 0, 100}}, 12};
	const auto scene = buildTrackScenery(track);
	for (float side : {-1.0f, 1.0f}) {
		REQUIRE(colorAt(scene, {1, side * 6.4f}) == white);
		REQUIRE(colorAt(scene, {3, side * 6.4f}) == black);
		REQUIRE(colorAt(scene, {5, side * 6.4f}) == white);
	}
	const Color asphalt{0.28f, 0.30f, 0.29f};
	REQUIRE(colorAt(scene, {3, 0}) == asphalt);
}

TEST_CASE("Les bandes epousent les deux sens de virage", "[scenery]") {
	for (int turn : {-1, 1}) {
		const TrackSegment segment{{0, 0}, 0, 80, true, 35, turn};
		const Track track{{segment}, 12};
		const auto scene = buildTrackScenery(track);
		const Vec2 center = Track::segmentPoint(segment, 3.2f);
		const float heading = Track::segmentHeading(segment, 3.2f);
		for (float side : {-1.0f, 1.0f}) {
			const float offset = side * 6.4f;
			const Vec2 point{center.x - std::sin(heading) * offset, center.y + std::cos(heading) * offset};
			REQUIRE(colorAt(scene, point) == black);
		}
	}
}

TEST_CASE("Les arbres restent eloignes de toutes les portions de piste", "[scenery]") {
	std::ifstream input{std::filesystem::path(F1_SOURCE_DIR) / "circuits/Le_Mans.f1"};
	const auto track = ChampionIO::read(input).track;
	const auto scene = buildTrackScenery(track);
	REQUIRE_FALSE(scene.trees.empty());
	REQUIRE(scene.trees.size() <= TrackScenery::maxTrees);
	for (const auto& tree : scene.trees) {
		CAPTURE(tree.position.x, tree.position.y);
		REQUIRE_FALSE(track.canDrive(tree.position, 0));
		const double clearance = tree.radius + TrackScenery::curbWidth + 2.0;
		REQUIRE(std::all_of(track.boundaries().begin(), track.boundaries().end(), [&](const auto& edge) {
			return distanceToEdge(tree.position, edge) >= clearance;
		}));
	}
	for (std::size_t i = 0; i < scene.trees.size(); ++i) {
		for (std::size_t j = i + 1; j < scene.trees.size(); ++j) {
			const auto& a = scene.trees[i];
			const auto& b = scene.trees[j];
			REQUIRE(std::hypot(a.position.x - b.position.x, a.position.y - b.position.y) >= a.radius + b.radius + 1.9f);
		}
	}
}

TEST_CASE("Le decor reste stable au rechargement et au deplacement du depart", "[scenery]") {
	const Track source{300, 200, 20};
	const auto before = buildTrackScenery(source);
	std::stringstream data;
	TrackIO::write(data, source);
	const auto loaded = TrackIO::read(data);
	REQUIRE(sameTrees(before, buildTrackScenery(loaded)));
	const Track movedStart{source.segments(), source.roadWidth(), source.startDistance() + 40};
	REQUIRE(sameTrees(before, buildTrackScenery(movedStart)));
	const Track wider{source.segments(), 24, source.startDistance()};
	REQUIRE_FALSE(sameTrees(before, buildTrackScenery(wider)));
	std::ostringstream after;
	TrackIO::write(after, source);
	REQUIRE(data.str() == after.str());
}

TEST_CASE("La vue globale peut cadrer tout le decor et les sommets restent finis", "[scenery][camera]") {
	const Track track{{TrackSegment{{90000, -90000}, 0.64f, 120}}, 12};
	const auto scene = buildTrackScenery(track);
	REQUIRE(scene.triangleVertexCount % 3 == 0);
	REQUIRE(scene.vertices.size() - scene.triangleVertexCount == track.boundaries().size() * 2 + 2);
	REQUIRE(std::all_of(scene.vertices.begin(), scene.vertices.end(), [](const auto& vertex) {
		return std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y) &&
			std::all_of(vertex.color.begin(), vertex.color.end(), [](float c) { return c >= 0 && c <= 1; });
	}));
	for (const auto dimensions : {Vec2{1600, 900}, Vec2{600, 1200}}) {
		const auto view = fitTrackView(track, static_cast<int>(dimensions.x), static_cast<int>(dimensions.y), TrackScenery::viewPadding);
		const float halfWidth = view.halfHeight * dimensions.x / dimensions.y;
		REQUIRE(std::all_of(scene.vertices.begin(), scene.vertices.end(), [&](const auto& vertex) {
			return std::abs(vertex.position.x - view.position.x) < halfWidth &&
				std::abs(vertex.position.y - view.position.y) < view.halfHeight;
		}));
	}
	const auto thin = buildTrackScenery(Track{{TrackSegment{{0, 0}, 0, 80}}, 1e-40f});
	REQUIRE(std::all_of(thin.vertices.begin(), thin.vertices.end(), [](const auto& vertex) {
		return std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y);
	}));
}

TEST_CASE("La taille du decor est bornee meme sur un tres long trace", "[scenery]") {
	const Track rectangle{9000, 6000, 20};
	std::vector<TrackSegment> segments;
	for (int i = 0; i < 16; ++i)
		segments.insert(segments.end(), rectangle.segments().begin(), rectangle.segments().end());
	const auto scene = buildTrackScenery(Track{segments, 20});
	REQUIRE(scene.vertices.size() < 500000);
	REQUIRE(scene.trees.size() <= TrackScenery::maxTrees);
	const auto empty = buildTrackScenery(Track{{}, 12});
	REQUIRE(empty.vertices.empty());
	REQUIRE(empty.trees.empty());
	REQUIRE(empty.triangleVertexCount == 0);
}
