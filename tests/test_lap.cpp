/* ****************************************** */
/*   test_lap.cpp                             */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/20 11:53:16 by Luc      */
/*   Updated: 2026/10/20 11:53:16 by Luc      */
/* ****************************************** */


#include "Track/LapCounter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Un tour exige les points de passage dans l'ordre", "[lap]") {
	LapCounter counter{320.0f, 0.0f};

	REQUIRE_FALSE(counter.update(80.0f, 1.0f));
	REQUIRE_FALSE(counter.update(160.0f, 1.0f));
	REQUIRE_FALSE(counter.update(240.0f, 1.0f));
	REQUIRE(counter.update(0.0f, 1.0f));

	REQUIRE(counter.lapCount() == 1);
	REQUIRE(counter.lastLapTime() == Catch::Approx(4.0f));
	REQUIRE(counter.currentLapTime()
			== Catch::Approx(0.0f).margin(1e-6f));

	// L'événement ne doit pas rester actif.
	REQUIRE_FALSE(counter.update(0.0f, 1.0f));
	REQUIRE(counter.lapCount() == 1);
}

TEST_CASE("Traverser la ligne en faisant demi-tour ne valide pas un tour",
		"[lap]") {
	LapCounter counter{320.0f, 0.0f};

	REQUIRE_FALSE(counter.update(10.0f, 1.0f));
	REQUIRE_FALSE(counter.update(319.0f, 1.0f));
	REQUIRE_FALSE(counter.update(1.0f, 1.0f));

	REQUIRE(counter.lapCount() == 0);
}

TEST_CASE("Un depart apres la ligne ne compte pas un tour partiel", "[lap]") {
	LapCounter counter{320.0f, 20.0f};

	REQUIRE_FALSE(counter.update(100.0f, 1.0f));
	REQUIRE_FALSE(counter.update(180.0f, 1.0f));
	REQUIRE_FALSE(counter.update(260.0f, 1.0f));

	// Premier passage sur la ligne : début du chronométrage.
	REQUIRE_FALSE(counter.update(0.0f, 1.0f));
	REQUIRE(counter.lapCount() == 0);

	REQUIRE_FALSE(counter.update(80.0f, 1.0f));
	REQUIRE_FALSE(counter.update(160.0f, 1.0f));
	REQUIRE_FALSE(counter.update(240.0f, 1.0f));
	REQUIRE(counter.update(0.0f, 1.0f));

	REQUIRE(counter.lapCount() == 1);
	REQUIRE(counter.lastLapTime() == Catch::Approx(4.0f));
}

TEST_CASE("Le chronometrage estime le franchissement entre deux pas",
		"[lap]") {
	LapCounter counter{320.0f, 0.0f};

	counter.update(80.0f, 1.0f);
	counter.update(160.0f, 1.0f);
	counter.update(240.0f, 1.0f);
	counter.update(310.0f, 1.0f);

	// Franchissement à mi-chemin entre 310 et 10.
	REQUIRE(counter.update(10.0f, 1.0f));

	REQUIRE(counter.lastLapTime() == Catch::Approx(4.5f));
	REQUIRE(counter.currentLapTime() == Catch::Approx(0.5f));
}

TEST_CASE("Reset efface les tours et les chronometres", "[lap]") {
	LapCounter counter{320.0f, 0.0f};

	counter.update(80.0f, 1.0f);
	counter.update(160.0f, 1.0f);
	counter.update(240.0f, 1.0f);
	REQUIRE(counter.update(0.0f, 1.0f));

	counter.reset(0.0f);

	REQUIRE(counter.lapCount() == 0);
	REQUIRE(counter.lastLapTime() == 0.0f);
	REQUIRE(counter.currentLapTime() == 0.0f);

	REQUIRE_FALSE(counter.update(80.0f, 1.0f));
	REQUIRE(counter.lapCount() == 0);
}