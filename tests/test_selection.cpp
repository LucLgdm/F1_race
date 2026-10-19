/* ****************************************** */
/*   test_selection.cpp                       */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/19 17:01:28 by Luc      */
/*   Updated: 2026/10/19 17:01:28 by Luc      */
/* ****************************************** */

#include "ML/Selection.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <span>
#include <stdexcept>

TEST_CASE("La selection retient le meilleur score", "[selection]") {
	const std::array<float, 4> scores{
		0.0f, 12.0f, 35.0f, 8.0f
	};

	REQUIRE(selectBestIndex(scores) == 2);
}

TEST_CASE("La selection fonctionne avec un seul candidat", "[selection]") {
	const std::array<float, 1> scores{15.0f};

	REQUIRE(selectBestIndex(scores) == 0);
}

TEST_CASE("La selection conserve le premier en cas d'egalite",
		"[selection]") {
	const std::array<float, 4> scores{
		5.0f, 20.0f, 20.0f, 3.0f
	};

	REQUIRE(selectBestIndex(scores) == 1);
}

TEST_CASE("La selection fonctionne si aucun agent ne progresse",
		"[selection]") {
	const std::array<float, 3> scores{0.0f, 0.0f, 0.0f};

	REQUIRE(selectBestIndex(scores) == 0);
}

TEST_CASE("La selection refuse une population vide", "[selection]") {
	REQUIRE_THROWS_AS(
		selectBestIndex(std::span<const float>{}),
		neuralError
	);
}