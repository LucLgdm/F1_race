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

TEST_CASE("Une probabilite nulle conserve les parametres", "[mutation]") {
	Neuron neuron({1.0f, -2.0f}, 0.5f, Activation::Linear);
	const std::array<float, 2> inputs{2.0f, 3.0f};

	const float before = neuron.forward(inputs);
	std::mt19937 generator{42};

	neuron.mutate(0.0f, 0.5f, generator);

	REQUIRE(neuron.forward(inputs) == before);
}

TEST_CASE("Une force nulle conserve les parametres", "[mutation]") {
	Neuron neuron({1.0f}, 0.5f, Activation::Linear);
	const std::array<float, 1> inputs{2.0f};

	const float before = neuron.forward(inputs);
	std::mt19937 generator{42};

	neuron.mutate(1.0f, 0.0f, generator);

	REQUIRE(neuron.forward(inputs) == before);
}

TEST_CASE("La mutation est reproductible avec la meme graine",
		"[mutation]") {
	Neuron first({1.0f, -2.0f}, 0.5f, Activation::Linear);
	Neuron second = first;

	std::mt19937 firstGenerator{42};
	std::mt19937 secondGenerator{42};

	first.mutate(0.5f, 0.1f, firstGenerator);
	second.mutate(0.5f, 0.1f, secondGenerator);

	const std::array<float, 2> inputs{2.0f, 3.0f};

	REQUIRE(first.forward(inputs) == second.forward(inputs));
}

TEST_CASE("Muter une copie ne modifie pas le parent", "[mutation]") {
	const Neuron parent({1.0f, -2.0f}, 0.5f, Activation::Linear);
	Neuron child = parent;

	std::mt19937 generator{42};

	child.mutate(1.0f, 0.5f, generator);

	const std::array<float, 2> inputs{2.0f, 3.0f};

	REQUIRE(parent.forward(inputs) == Catch::Approx(-3.5f));

	// Vérifier aussi le biais et les poids du parent séparément.
	REQUIRE(parent.forward(std::array<float, 2>{0.0f, 0.0f})
			== Catch::Approx(0.5f));

	REQUIRE(parent.forward(std::array<float, 2>{1.0f, 0.0f})
			== Catch::Approx(1.5f));

	REQUIRE(parent.forward(std::array<float, 2>{0.0f, 1.0f})
			== Catch::Approx(-1.5f));
}

TEST_CASE("La mutation refuse des parametres invalides", "[mutation]") {
	Neuron neuron({1.0f}, 0.0f, Activation::Linear);
	std::mt19937 generator{42};

	REQUIRE_THROWS_AS(neuron.mutate(-0.1f, 0.1f, generator), neuralError);
	REQUIRE_THROWS_AS(neuron.mutate(1.1f, 0.1f, generator), neuralError);
	REQUIRE_THROWS_AS(neuron.mutate(0.1f, -0.1f, generator), neuralError);
}