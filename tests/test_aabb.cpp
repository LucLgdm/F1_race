#include "Track/Aabb2D.hpp"

#include <catch2/catch_test_macros.hpp>
#include <array>

TEST_CASE("Une AABB vide ne contient aucun point et ne recouvre aucune boite", "[aabb]") {
	const Aabb2D empty;
	const Aabb2D bounds{{-8, -6}, {-2, -1}};
	REQUIRE_FALSE(empty.contains({0, 0}));
	REQUIRE_FALSE(empty.contains({-5, -3}));
	REQUIRE_FALSE(empty.overlaps(bounds));
	REQUIRE_FALSE(bounds.overlaps(empty));
	REQUIRE_FALSE(empty.overlaps(empty));
}

TEST_CASE("Le premier point definit les deux extremites de l AABB", "[aabb]") {
	Aabb2D bounds;
	bounds.expand({-8, -6});
	REQUIRE(bounds.min.x == -8);
	REQUIRE(bounds.min.y == -6);
	REQUIRE(bounds.max.x == -8);
	REQUIRE(bounds.max.y == -6);
	REQUIRE(bounds.contains({-8, -6}));
	REQUIRE_FALSE(bounds.contains({-8, -5}));
}

TEST_CASE("Une AABB englobe des points negatifs puis s agrandit dans chaque direction", "[aabb]") {
	Aabb2D bounds;
	bounds.expand({-8, -6});
	bounds.expand({-2, -1});
	bounds.expand({-5, -3});
	REQUIRE(bounds.min.x == -8);
	REQUIRE(bounds.min.y == -6);
	REQUIRE(bounds.max.x == -2);
	REQUIRE(bounds.max.y == -1);
	REQUIRE(bounds.contains({-5, -3}));
	REQUIRE_FALSE(bounds.contains({0, 0}));
	bounds.expand({3, -9});
	REQUIRE(bounds.min.x == -8);
	REQUIRE(bounds.min.y == -9);
	REQUIRE(bounds.max.x == 3);
	REQUIRE(bounds.max.y == -1);
}

TEST_CASE("Une AABB inclut son interieur ses bords et ses coins", "[aabb]") {
	const Aabb2D bounds{{-8, -6}, {-2, -1}};
	const std::array<Vec2, 9> inside{{
		{-5, -3}, {-8, -6}, {-2, -6}, {-8, -1}, {-2, -1},
		{-8, -3}, {-2, -3}, {-5, -6}, {-5, -1}
	}};
	const std::array<Vec2, 4> outside{{{-9, -3}, {-1, -3}, {-5, -7}, {-5, 0}}};
	for (const auto point : inside)
		REQUIRE(bounds.contains(point));
	for (const auto point : outside)
		REQUIRE_FALSE(bounds.contains(point));
}

TEST_CASE("Le recouvrement des AABB est symetrique et conserve les contacts", "[aabb]") {
	const Aabb2D bounds{{0, 0}, {4, 4}};
	struct Example {
		Aabb2D other;
		bool expected;
	};
	const std::array<Example, 10> examples{{
		{{{0, 0}, {4, 4}}, true},
		{{{1, 1}, {3, 3}}, true},
		{{{-1, -1}, {5, 5}}, true},
		{{{2, 2}, {6, 6}}, true},
		{{{4, 1}, {6, 3}}, true},
		{{{1, 4}, {3, 6}}, true},
		{{{4, 4}, {6, 6}}, true},
		{{{5, 1}, {6, 3}}, false},
		{{{1, 5}, {3, 6}}, false},
		{{{-3, -3}, {-1, -1}}, false}
	}};
	for (const auto& example : examples) {
		CAPTURE(example.other.min.x, example.other.min.y);
		REQUIRE(bounds.overlaps(example.other) == example.expected);
		REQUIRE(example.other.overlaps(bounds) == example.expected);
	}
}

TEST_CASE("Une AABB peut entourer un segment horizontal vertical ou un point", "[aabb]") {
	const Aabb2D horizontal{{-2, 1}, {2, 1}};
	const Aabb2D vertical{{0, -1}, {0, 3}};
	const Aabb2D point{{0, 1}, {0, 1}};
	REQUIRE(horizontal.contains({0, 1}));
	REQUIRE_FALSE(horizontal.contains({0, 2}));
	REQUIRE(vertical.contains({0, 1}));
	REQUIRE_FALSE(vertical.contains({1, 1}));
	REQUIRE(horizontal.overlaps(vertical));
	REQUIRE(horizontal.overlaps(point));
	REQUIRE(vertical.overlaps(point));
	REQUIRE(point.overlaps(point));
}
