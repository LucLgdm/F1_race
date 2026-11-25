#include "TrackQueryReference.hpp"
#include "Track/ChampionIO.hpp"
#include "Track/TrackIO.hpp"
#include <fstream>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <limits>
#include <numbers>
#include <random>

namespace {
constexpr float pi = std::numbers::pi_v<float>;

Track roundedTrack(int turn, Vec2 start = {-300, -200}) {
	std::vector<TrackSegment> segments;
	TrackSegment segment{start, 0.37f, 150};
	for (int i = 0; i < 4; ++i) {
		segment.rounded = false;
		segments.push_back(segment);
		segment.start = Track::segmentPoint(segment, Track::segmentLength(segment));
		segment.rounded = true;
		segment.radius = 65;
		segment.turn = turn;
		segments.push_back(segment);
		segment.start = Track::segmentPoint(segment, Track::segmentLength(segment));
		segment.heading = Track::segmentHeading(segment, Track::segmentLength(segment));
	}
	return Track{segments, 12, 35};
}

std::vector<Track> examples() {
	std::vector<Track> tracks;
	tracks.emplace_back(120, 80, 20);
	tracks.emplace_back(std::vector<TrackSegment>{{{-50, -70}, 0.64f, 120}}, 12);
	tracks.push_back(roundedTrack(-1));
	tracks.push_back(roundedTrack(1));
	tracks.emplace_back(std::vector<TrackSegment>{{{0, 0}, 0, 40}, {{40.001f, 0}, 0, 40}}, 10);
	tracks.emplace_back(std::vector<TrackSegment>{{{0, 0}, 0, 80, true, 50, 1}}, 99.99f);
	tracks.emplace_back(std::vector<TrackSegment>{{{90000, -90000}, 0.64f, 120}}, 12);
	tracks.emplace_back(std::vector<TrackSegment>{{{0, 0}, 0, 80}}, 0.000001f);
	std::ifstream input{std::filesystem::path(F1_SOURCE_DIR) / "circuits/Le_Mans.f1"};
	tracks.push_back(ChampionIO::read(input).track);
	tracks.emplace_back(std::vector<TrackSegment>{}, 12);
	return tracks;
}
} // namespace

TEST_CASE("Les recherches BVH donnent les memes resultats que les recherches exhaustives", "[bvh][track]") {
	std::mt19937 generator{20261123};
	std::uniform_real_distribution<float> unit{0, 1};
	const auto tracks = examples();
	for (std::size_t index = 0; index < tracks.size(); ++index) {
		const auto& track = tracks[index];
		for (int sample = 0; sample < 250; ++sample) {
			CAPTURE(index, sample);
			const float coordinate = unit(generator) * track.length();
			const float heading = track.headingAt(coordinate);
			const auto center = track.pointAt(coordinate);
			const float lateral = (unit(generator) - 0.5f) * 1.6f * track.roadWidth();
			const Vec2 point{center.x - std::sin(heading) * lateral,
							 center.y + std::cos(heading) * lateral};
			const float radius = std::array{0.0f, 0.001f, 2.3f, track.roadWidth() / 2}[sample % 4];
			const float angle = heading + (unit(generator) * 2 - 1) * pi;
			const float range = std::array{0.00001f, 0.1f, 40.0f, 1000.0f}[sample % 4];
			const auto next = track.pointAt(coordinate + (unit(generator) - 0.5f) * 160);
			REQUIRE(track.canDrive(point, radius) == track_reference::canDrive(track, point, radius));
			REQUIRE(track.canDrive(point, 0) == track_reference::contains(track, point));
			REQUIRE(track.raycast(point, angle, range) == track_reference::raycast(track, point, angle, range));
			REQUIRE(track.canTravel(point, next, radius) == track_reference::canTravel(track, point, next, radius));
		}
	}
}

TEST_CASE("Le filtrage BVH conserve les tolerances aux bords et aux raccords", "[bvh][track]") {
	const auto tracks = examples();
	for (const auto& track : tracks) {
		const auto& edges = track.boundaries();
		for (std::size_t i = 0; i < edges.size(); i += std::max(std::size_t{1}, edges.size() / 50)) {
			const auto& edge = edges[i];
			const Vec2 delta{edge.b.x - edge.a.x, edge.b.y - edge.a.y};
			const float length = std::hypot(delta.x, delta.y);
			if (length == 0)
				continue;
			const float heading = std::atan2(delta.y, delta.x);
			for (float fraction : {0.0f, 0.5f, 1.0f}) {
				for (float offset : {-0.00001f, 0.0f, 0.00001f}) {
					const Vec2 point{edge.a.x + delta.x * fraction - delta.y / length * offset,
									 edge.a.y + delta.y * fraction + delta.x / length * offset};
					CAPTURE(i, fraction, offset, point.x, point.y);
					REQUIRE(track.canDrive(point, 0) == track_reference::contains(track, point));
					for (float angle : {heading, heading + pi / 2, heading + pi})
						REQUIRE(track.raycast(point, angle, 40) == track_reference::raycast(track, point, angle, 40));
				}
			}
		}
	}
}

TEST_CASE("Les collisions balayees conservent les obstacles et les contacts tangents", "[bvh][track]") {
	const Track track{120, 80, 20};
	REQUIRE_FALSE(track.canTravel({10, 30}, {10, -30}, 2.3f));
	for (float radius : {0.0f, 2.3f})
		for (float y : {20.0f + radius, 30.0f, 40.0f - radius})
			REQUIRE(track.canTravel({-20, y}, {20, y}, radius) ==
					track_reference::canTravel(track, {-20, y}, {20, y}, radius));
	const Track open{{TrackSegment{{0, 0}, 0, 100}}, 10};
	REQUIRE(open.raycast({0.000001f, 0}, 0, 40) == 0);
	REQUIRE(open.raycast({-0.000001f, 0}, 0, 40) ==
			track_reference::raycast(open, {-0.000001f, 0}, 0, 40));
	REQUIRE(open.raycast({50, 0}, pi / 2, 1) == 1);
	REQUIRE(open.canTravel({50, 0}, {50, 0}, 2.3f));
}

TEST_CASE("Les copies et remplacements de circuits conservent leur propre BVH", "[bvh][track]") {
	Track source = roundedTrack(-1);
	Track copy = source;
	const Vec2 point = source.pointAt(10);
	const float expected = source.raycast(point, 0, 40);
	source = Track{{}, 12};
	REQUIRE(copy.raycast(point, 0, 40) == expected);
	REQUIRE(source.raycast(point, 0, 40) == 0);
	copy = Track{120, 80, 20};
	REQUIRE(copy.canDrive({0, 30}, 2.3f));
	REQUIRE_FALSE(copy.canDrive(point, 2.3f));
	copy = Track{{}, 12};
	REQUIRE_FALSE(copy.canDrive({0, 30}, 2.3f));
}

TEST_CASE("Les recherches indexees conservent la validation des arguments", "[bvh][track]") {
	const Track track{120, 80, 20};
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	REQUIRE_FALSE(track.canDrive({nan, 30}, 2.3f));
	REQUIRE_FALSE(track.canDrive({0, 30}, -1));
	REQUIRE_FALSE(track.canTravel({0, 30}, {inf, 30}, 2.3f));
	REQUIRE_THROWS_AS(track.raycast({0, 30}, nan, 40), trackError);
	REQUIRE_THROWS_AS(track.raycast({0, 30}, 0, inf), trackError);
	REQUIRE_THROWS_AS(track.raycast({nan, 30}, 0, 40), trackError);
	REQUIRE_THROWS_AS(track.raycast({0, 30}, 0, 0), trackError);
	const float range = std::numeric_limits<float>::max();
	REQUIRE(track.raycast({0, 30}, 0, range) == track_reference::raycast(track, {0, 30}, 0, range));
}
