/* ****************************************** */
/*   test_champion_io.cpp                     */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/20 15:28:33 by Luc      */
/*   Updated: 2026/10/20 15:28:33 by Luc      */
/* ****************************************** */

#include "Track/ChampionIO.hpp"
#include "Exception.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <sstream>
#include <vector>

TEST_CASE("Un champion recharge conserve ses predictions et son circuit", "[champion]") {
	const Track track{800.0f, 500.0f, 20.0f};

	const Network network{
		std::vector<Layer>{
			Layer{8, 4, Activation::Tanh},
			Layer{4, 3, Activation::Linear}
		}
	};

	std::stringstream data;

	ChampionIO::write(data, track, network, 49.7255f);

	SavedChampion loaded = ChampionIO::read(data);

	REQUIRE(loaded.lapTime == 49.7255f);
	REQUIRE(loaded.track.length() == Catch::Approx(2520.0f));

	REQUIRE(loaded.track.startPosition().x
			== track.startPosition().x);

	REQUIRE(loaded.track.startPosition().y
			== track.startPosition().y);

	const std::array<std::array<float, 8>, 3> examples{{
		{{0, 0, 0, 0, 0, 0, 0, 0}},
		{{1, 1, 1, 1, 1, 1, 0, 1}},
		{{0.2f, 0.4f, 0.8f, 0.3f, 0.1f, 0.5f, -0.6f, 0.8f}}
	}};

	for (const auto& inputs : examples) {
		const auto expected = network.forward(inputs);
		const auto actual = loaded.network.forward(inputs);

		REQUIRE(actual.size() == expected.size());

		for (std::size_t i = 0; i < actual.size(); ++i)
			REQUIRE(actual[i] == expected[i]);
	}
}

TEST_CASE("La lecture refuse une version inconnue", "[champion]") {
	std::stringstream data{"F1_CHAMPION 99\n"};

	REQUIRE_THROWS_AS(
		ChampionIO::read(data),
		IOError
	);
}

TEST_CASE("La lecture refuse un fichier tronque", "[champion]") {
	std::stringstream data{
		"F1_CHAMPION 1\n"
		"RECTANGLE 800 500 20\n"
	};

	REQUIRE_THROWS_AS(
		ChampionIO::read(data),
		IOError
	);
}

TEST_CASE("La sauvegarde refuse un record non positif", "[champion]") {
	const Track track{800.0f, 500.0f, 20.0f};

	const Network network{
		std::vector<Layer>{
			Layer{8, 3, Activation::Tanh}
		}
	};

	std::stringstream data;

	REQUIRE_THROWS_AS(
		ChampionIO::write(data, track, network, 0.0f),
		IOError
	);
}