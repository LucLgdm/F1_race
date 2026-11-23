#include "Track/Bvh2D.hpp"
#include <catch2/catch_test_macros.hpp>
#include <utility>

TEST_CASE("Un BVH vide ne visite aucune primitive", "[bvh]") {
	Bvh2D tree;
	int visits = 0;
	REQUIRE_FALSE(tree.anyOverlapping({{-1, -1}, {1, 1}}, [&](std::size_t) {
		++visits;
		return true;
	}));
	REQUIRE(visits == 0);
}

TEST_CASE("Le BVH conserve les indices et elimine les groupes eloignes", "[bvh]") {
	std::vector<Aabb2D> bounds;
	for (int y = 0; y < 32; ++y)
		for (int x = 0; x < 32; ++x)
			bounds.push_back({{float(x * 10), float(y * 10)}, {float(x * 10 + 2), float(y * 10 + 2)}});
	Bvh2D tree;
	tree.build(bounds);
	for (std::size_t target = 0; target < bounds.size(); target += 31) {
		CAPTURE(target);
		int visits = 0;
		REQUIRE(tree.anyOverlapping(bounds[target], [&](std::size_t index) {
			++visits;
			return index == target;
		}));
		REQUIRE(visits < 32);
	}
}

TEST_CASE("Le BVH visite tous les candidats et s arrete sur un resultat", "[bvh]") {
	const std::vector<Aabb2D> bounds(100, {{-1, -1}, {1, 1}});
	Bvh2D tree;
	tree.build(bounds);
	std::vector<bool> visited(bounds.size(), false);
	int count = 0;
	tree.forEachOverlapping({{0, 0}, {0, 0}}, [&](std::size_t index) {
		REQUIRE(index < bounds.size());
		REQUIRE_FALSE(visited[index]);
		visited[index] = true;
		++count;
	});
	REQUIRE(count == 100);
	count = 0;
	REQUIRE(tree.anyOverlapping({{0, 0}, {0, 0}}, [&](std::size_t) {
		++count;
		return true;
	}));
	REQUIRE(count == 1);
}

TEST_CASE("Le BVH conserve les contacts et peut etre reconstruit copie et deplace", "[bvh]") {
	Bvh2D tree;
	const std::vector<Aabb2D> bounds{{{-8, -6}, {-2, -1}}, {{10, 10}, {10, 10}}};
	tree.build(bounds);
	Bvh2D copy = tree;
	tree.clear();
	Bvh2D moved = std::move(copy);
	REQUIRE(moved.anyOverlapping({{-2, -1}, {0, 0}}, [](std::size_t i) { return i == 0; }));
	REQUIRE(moved.anyOverlapping({{10, 10}, {10, 10}}, [](std::size_t i) { return i == 1; }));
	const std::vector<Aabb2D> replacement{{{50, 50}, {51, 51}}};
	moved.build(replacement);
	REQUIRE_FALSE(moved.anyOverlapping({{-2, -1}, {0, 0}}, [](std::size_t) { return true; }));
	REQUIRE(moved.anyOverlapping({{50, 50}, {51, 51}}, [](std::size_t i) { return i == 0; }));
	moved.build({});
	REQUIRE_FALSE(moved.anyOverlapping({{50, 50}, {51, 51}}, [](std::size_t) { return true; }));
}
