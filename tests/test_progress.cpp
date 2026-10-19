/* ****************************************** */
/*   test_progress.cpp                        */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/16 14:38:34 by Luc      */
/*   Updated: 2026/10/16 14:38:34 by Luc      */
/* ****************************************** */


#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "ML/ProgressTracker.hpp"

TEST_CASE("La progression commence a zero quelle que soit la grille", "[progress]") {
	ProgressTracker first{320.0f, 28.5f};
	ProgressTracker second{320.0f, 291.5f};

	REQUIRE(first.progress() == 0.0f);
	REQUIRE(second.progress() == 0.0f);
	REQUIRE(first.bestProgress() == 0.0f);
	REQUIRE(second.bestProgress() == 0.0f);

	first.update(33.5f, 0.1f);
	second.update(296.5f, 0.1f);

	REQUIRE(first.progress() == Catch::Approx(5.0f));
	REQUIRE(second.progress() == Catch::Approx(5.0f));
}

TEST_CASE("La progression gere le passage de la ligne dans les deux sens", "[progress]") {
	SECTION("Sens horaire") {
		ProgressTracker tracker{320.0f, 319.0f};

		tracker.update(1.0f, 0.1f);

		REQUIRE(tracker.progress() == Catch::Approx(2.0f));
		REQUIRE(tracker.bestProgress() == Catch::Approx(2.0f));
	}

	SECTION("Contre-sens") {
		ProgressTracker tracker{320.0f, 1.0f};

		tracker.update(319.0f, 0.1f);

		REQUIRE(tracker.progress() == Catch::Approx(-2.0f));
		REQUIRE(tracker.bestProgress() == 0.0f);
	}
}

TEST_CASE("Revenir sur ses pas ne rapporte pas une nouvelle progression", "[progress]") {
	ProgressTracker tracker{320.0f, 20.0f};

	tracker.update(30.0f, 0.1f);
	tracker.update(25.0f, 0.1f);

	REQUIRE(tracker.progress() == Catch::Approx(5.0f));
	REQUIRE(tracker.bestProgress() == Catch::Approx(10.0f));

	tracker.update(30.0f, 0.1f);

	REQUIRE(tracker.progress() == Catch::Approx(10.0f));
	REQUIRE(tracker.bestProgress() == Catch::Approx(10.0f));
}

TEST_CASE("La progression cumule plusieurs passages par zero", "[progress]") {
	ProgressTracker tracker{320.0f, 0.0f};

	// Chaque déplacement reste inférieur à un demi-tour.
	for (int lap = 0; lap < 2; ++lap) {
		tracker.update(80.0f, 0.1f);
		tracker.update(160.0f, 0.1f);
		tracker.update(240.0f, 0.1f);
		tracker.update(0.0f, 0.1f);
	}

	REQUIRE(tracker.progress() == Catch::Approx(640.0f));
	REQUIRE(tracker.bestProgress() == Catch::Approx(640.0f));
}

TEST_CASE("Reset efface la progression et utilise le nouveau depart", "[progress]") {
	ProgressTracker tracker{320.0f, 20.0f};

	tracker.update(40.0f, 0.1f);
	tracker.reset(310.0f);

	REQUIRE(tracker.progress() == 0.0f);
	REQUIRE(tracker.bestProgress() == 0.0f);

	tracker.update(2.0f, 0.1f);

	REQUIRE(tracker.progress() == Catch::Approx(12.0f));
	REQUIRE(tracker.bestProgress() == Catch::Approx(12.0f));
}

TEST_CASE("Un agent immobile atteint le delai sans progression",
          "[progress]") {
    ProgressTracker tracker{320.0f, 20.0f};

    tracker.update(20.0f, 1.0f);
    tracker.update(20.0f, 1.0f);

    REQUIRE_FALSE(tracker.hasStalled(3.0f));

    tracker.update(20.0f, 1.0f);

    REQUIRE(tracker.hasStalled(3.0f));
    REQUIRE(tracker.timeWithoutProgress() == Catch::Approx(3.0f));
}

TEST_CASE("Les petites avancees cumulees renouvellent le delai",
          "[progress]") {
    ProgressTracker tracker{320.0f, 0.0f};

    tracker.update(0.2f, 1.0f);
    tracker.update(0.4f, 1.0f);

    REQUIRE(tracker.timeWithoutProgress() == Catch::Approx(2.0f));

    tracker.update(0.6f, 0.5f);

    REQUIRE(tracker.bestProgress() == Catch::Approx(0.6f));
    REQUIRE(tracker.timeWithoutProgress()
            == Catch::Approx(0.0f).margin(1e-6f));
    REQUIRE_FALSE(tracker.hasStalled(3.0f));
}

TEST_CASE("Revenir au record precedent ne renouvelle pas le delai",
          "[progress]") {
    ProgressTracker tracker{320.0f, 20.0f};

    tracker.update(21.0f, 0.5f); // Nouveau record : délai renouvelé.
    tracker.update(20.0f, 1.5f); // Recule.
    tracker.update(21.0f, 1.5f); // Revient au même record.

    REQUIRE(tracker.bestProgress() == Catch::Approx(1.0f));
    REQUIRE(tracker.hasStalled(3.0f));
}

TEST_CASE("Reset efface aussi le delai et le seuil de progression",
          "[progress]") {
    ProgressTracker tracker{320.0f, 20.0f};

    tracker.update(25.0f, 1.0f);
    tracker.update(25.0f, 3.0f);

    REQUIRE(tracker.hasStalled(3.0f));

    tracker.reset(100.0f);

    REQUIRE_FALSE(tracker.hasStalled(3.0f));
    REQUIRE(tracker.timeWithoutProgress() == 0.0f);

    tracker.update(100.6f, 1.0f);

    REQUIRE(tracker.timeWithoutProgress()
            == Catch::Approx(0.0f).margin(1e-6f));
}