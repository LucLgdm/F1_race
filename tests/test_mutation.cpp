/* ****************************************** */
/*   test_mutation.cpp                        */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/19 17:51:48 by Luc      */
/*   Updated: 2026/10/19 17:51:48 by Luc      */
/* ****************************************** */

#include "ML/Network.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <random>
#include <vector>

TEST_CASE("La mutation traverse les couches sans modifier le parent",
		"[mutation]") {
	const Network parent{
		std::vector<Layer>{
			Layer{
				std::vector<Neuron>{
					Neuron{{1.0f}, 0.0f, Activation::Linear}
				}
			},
			Layer{
				std::vector<Neuron>{
					Neuron{{1.0f}, 0.0f, Activation::Linear}
				}
			}
		}
	};

	Network child = parent;
	std::mt19937 generator{42};

	// Mutation de tous les paramètres pour ce test.
	child.mutate(1.0f, 0.5f, generator);

	const std::array<float, 1> zero{0.0f};
	const std::array<float, 1> one{1.0f};

	REQUIRE(parent.forward(zero).front() == 0.0f);
	REQUIRE(parent.forward(one).front() == 1.0f);

	// Le comportement du descendant doit avoir changé.
	const bool changed =
		child.forward(zero).front() != parent.forward(zero).front()
		|| child.forward(one).front() != parent.forward(one).front();

	REQUIRE(changed);
}