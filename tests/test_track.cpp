/* ****************************************** */
/*   test_track.cpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/16 13:38:33 by Luc      */
/*   Updated: 2026/10/16 13:38:33 by Luc      */
/* ****************************************** */

#include "Track/Track.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>

TEST_CASE("La longueur suit la ligne centrale du circuit", "[track]") {
	SECTION("Route de 20 metres") {
		const Track track{120.0f, 80.0f, 20.0f};

		// Ligne centrale : 100 × 60 ; perimetre = 320.
		REQUIRE(track.length() == Catch::Approx(320.0f));
	}

	SECTION("Une route plus etroite change la ligne centrale") {
		const Track track{120.0f, 80.0f, 10.0f};

		// Ligne centrale : 110 × 70 ; perimetre = 360.
		REQUIRE(track.length() == Catch::Approx(360.0f));
	}

	SECTION("Autres dimensions") {
		const Track track{300.0f, 200.0f, 15.0f};

		REQUIRE(track.length() == Catch::Approx(940.0f));
	}
}

TEST_CASE("La progression est nulle au depart", "[track]") {
	const Track track{120.0f, 80.0f, 20.0f};

	REQUIRE(track.progressAt(track.startPosition())
			== Catch::Approx(0.0f).margin(1e-4f));
}

TEST_CASE("La progression suit le circuit dans le sens horaire", "[track]") {
	const Track track{120.0f, 80.0f, 20.0f};

	struct Example {
		Vec2 position;
		float expected;
	};

	// Ligne centrale : x = ±50, y = ±30.
	const std::array<Example, 8> examples{{
		{{ 10.0f,  30.0f},  10.0f}, // Ligne droite superieure.
		{{ 50.0f,  30.0f},  50.0f}, // Coin superieur droit.
		{{ 50.0f,   0.0f},  80.0f}, // Cote droit.
		{{ 50.0f, -30.0f}, 110.0f}, // Coin inferieur droit.
		{{  0.0f, -30.0f}, 160.0f}, // Ligne droite inferieure.
		{{-50.0f, -30.0f}, 210.0f}, // Coin inferieur gauche.
		{{-50.0f,   0.0f}, 240.0f}, // Cote gauche.
		{{-50.0f,  30.0f}, 270.0f}  // Coin superieur gauche.
	}};

	for (const auto& example : examples) {
		CAPTURE(example.position.x, example.position.y);

		REQUIRE(track.progressAt(example.position)
				== Catch::Approx(example.expected).margin(1e-4f));
	}
}

TEST_CASE("La progression revient a zero a la ligne de depart", "[track]") {
	const Track track{120.0f, 80.0f, 20.0f};

	SECTION("Dix metres avant le depart") {
		REQUIRE(track.progressAt(Vec2{-10.0f, 30.0f})
				== Catch::Approx(310.0f).margin(1e-4f));
	}

	SECTION("Un metre avant le depart") {
		REQUIRE(track.progressAt(Vec2{-1.0f, 30.0f})
				== Catch::Approx(319.0f).margin(1e-4f));
	}

	SECTION("Un metre apres le depart") {
		REQUIRE(track.progressAt(Vec2{1.0f, 30.0f})
				== Catch::Approx(1.0f).margin(1e-4f));
	}
}

TEST_CASE("La progression projette la voiture sur la ligne centrale",
		"[track]") {
	const Track track{120.0f, 80.0f, 20.0f};

	struct Example {
		Vec2 position;
		float expected;
	};

	// Deux positions laterales par ligne droite.
	const std::array<Example, 8> examples{{
		{{ 10.0f,  25.0f},  10.0f},
		{{ 10.0f,  35.0f},  10.0f},
		{{ 45.0f,   0.0f},  80.0f},
		{{ 55.0f,   0.0f},  80.0f},
		{{  0.0f, -25.0f}, 160.0f},
		{{  0.0f, -35.0f}, 160.0f},
		{{-45.0f,   0.0f}, 240.0f},
		{{-55.0f,   0.0f}, 240.0f}
	}};

	for (const auto& example : examples) {
		CAPTURE(example.position.x, example.position.y);

		REQUIRE(track.progressAt(example.position)
				== Catch::Approx(example.expected).margin(1e-4f));
	}
}