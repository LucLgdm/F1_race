/* ****************************************** */
/*   test_neuron.cpp                          */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/15 15:51:34 by Luc      */
/*   Updated: 2026/10/15 15:51:34 by Luc      */
/* ****************************************** */

#include <array>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "ML/Neuron.hpp"

TEST_CASE("Le neurone calcule sa somme ponderee", "[neuron]") {
	const Neuron neuron({0.4f, -0.2f}, 0.1f, Activation::Linear);
	const std::array<float, 2> inputs{0.5f, 0.8f};

	// 0.4 × 0.5 - 0.2 × 0.8 + 0.1 = 0.14
	REQUIRE(neuron.weightedSum(inputs) == Catch::Approx(0.14f));
	REQUIRE(neuron.forward(inputs) == Catch::Approx(0.14f));
}

TEST_CASE("ReLU annule une somme negative", "[neuron]") {
	const Neuron neuron({1.0f, -2.0f}, 0.0f, Activation::ReLU);
	const std::array<float, 2> inputs{1.0f, 1.0f};

	// Somme = -1 ; ReLU(-1) = 0.
	REQUIRE(neuron.weightedSum(inputs) == Catch::Approx(-1.0f));
	REQUIRE(neuron.forward(inputs) == 0.0f);
}

TEST_CASE("ReLU conserve une somme positive", "[neuron]") {
	const Neuron neuron({1.0f, 2.0f}, 0.5f, Activation::ReLU);
	const std::array<float, 2> inputs{1.0f, 1.0f};

	REQUIRE(neuron.forward(inputs) == Catch::Approx(3.5f));
}

TEST_CASE("Le neurone refuse un nombre incorrect d'entrees", "[neuron]") {
	const Neuron neuron({0.4f, -0.2f}, 0.1f);
	const std::array<float, 1> inputs{0.5f};

	REQUIRE_THROWS_AS(neuron.forward(inputs), neuralError);
}

/***************************************************************
 * Calcul	Résultat
 * Sortie initiale : \(1 \times 2 + 0\)	\(2\)
 * Gradient de l’erreur : \(2 \times (2 - 0)\)	\(4\)
 * Nouveau poids : \(1 - 0.1 \times 4 \times 2\)	\(0.2\)
 * Nouveau biais : \(0 - 0.1 \times 4\)	\(-0.4\)
 * Nouvelle sortie : \(0.2 \times 2 - 0.4\)	\(0\)
 ***************************************************************/
TEST_CASE("Le neurone effectue une etape d'apprentissage", "[neuron]") {
	Neuron neuron({1.0f}, 0.0f, Activation::Linear);
	const std::array<float, 1> inputs{2.0f};

	REQUIRE(neuron.forward(inputs) == Catch::Approx(2.0f));

	neuron.train(inputs, 0.0f, 0.1f);

	REQUIRE(neuron.forward(inputs)
			== Catch::Approx(0.0f).margin(1e-6f));
}

