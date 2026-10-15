/* ****************************************** */
/*   test_loss.cpp                            */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/15 15:42:59 by Luc      */
/*   Updated: 2026/10/15 15:42:59 by Luc      */
/* ****************************************** */

#include <array>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "ML/Loss.hpp"

TEST_CASE("La MSE est nulle pour des predictions exactes", "[loss]") {
    const std::array<float, 2> predictions{0.2f, 0.8f};
    const std::array<float, 2> targets{0.2f, 0.8f};

    REQUIRE(meanSquaredError(predictions, targets)
            == Catch::Approx(0.0f).margin(1e-6f));
}

TEST_CASE("La MSE correspond au calcul attendu", "[loss]") {
    const std::array<float, 2> predictions{1.0f, 3.0f};
    const std::array<float, 2> targets{2.0f, 1.0f};

    // ((1 - 2)² + (3 - 1)²) / 2 = 2.5
    REQUIRE(meanSquaredError(predictions, targets)
            == Catch::Approx(2.5f));
}

TEST_CASE("La MSE refuse des tailles differentes", "[loss]") {
    const std::array<float, 1> predictions{0.5f};
    const std::array<float, 2> targets{0.5f, 0.6f};

    REQUIRE_THROWS_AS(
        meanSquaredError(predictions, targets),
        neuralError
    );
}

TEST_CASE("La MSE refuse des entrees vides", "[loss]") {
    REQUIRE_THROWS_AS(
        meanSquaredError(std::span<const float>{},
                         std::span<const float>{}),
        neuralError
    );
}
