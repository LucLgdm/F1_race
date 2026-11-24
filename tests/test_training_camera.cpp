#include "TrainingCamera.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

namespace {
Network constantControls(float throttle, float brake = 0.0f) {
	return Network{std::vector<Layer>{Layer{std::vector<Neuron>{
		Neuron{std::vector<float>(8, 0), 0, Activation::Linear},
		Neuron{std::vector<float>(8, 0), throttle, Activation::Linear},
		Neuron{std::vector<float>(8, 0), brake, Activation::Linear}
	}}}};
}

void advance(Agent& agent, int steps) {
	for (int i = 0; i < steps; ++i)
		agent.step(1.0f / 120.0f);
}
} // namespace

TEST_CASE("La camera choisit la tete de la grille plutot que le premier indice", "[camera][training]") {
	const Track track{800, 500, 20};
	std::vector<Agent> agents;
	for (float coordinate : {-18.0f, -12.0f, -6.0f})
		agents.emplace_back(track, track.pointAt(coordinate), coordinate);
	REQUIRE(selectTrainingCameraTarget(agents) == 2);
	for (const auto& agent : agents)
		REQUIRE(agent.progress() == 0);
}

TEST_CASE("La camera suit la tete meme si une voiture derriere a parcouru plus de distance", "[camera][training]") {
	const Track track{800, 500, 20};
	std::vector<Agent> agents;
	agents.emplace_back(track, track.pointAt(-6), -6);
	agents.emplace_back(track, track.pointAt(-12), -12);
	agents[0].setNetwork(constantControls(0.2f));
	agents[1].setNetwork(constantControls(1));
	advance(agents[0], 120);
	advance(agents[1], 120);
	REQUIRE(agents[0].isActive());
	REQUIRE(agents[1].isActive());
	REQUIRE(agents[1].progress() > agents[0].progress());
	REQUIRE(agents[0].racePosition() > agents[1].racePosition());
	REQUIRE(selectTrainingCameraTarget(agents, 0) == 0);

	advance(agents[0], 120);
	advance(agents[1], 120);
	REQUIRE(agents[1].isActive());
	REQUIRE(agents[1].racePosition() > 0);
	REQUIRE(agents[1].racePosition() > agents[0].racePosition());
	REQUIRE(selectTrainingCameraTarget(agents, 0) == 1);
}

TEST_CASE("La camera quitte un agent arrete et conserve sa cible si tous sont arretes", "[camera][training]") {
	const Track track{800, 500, 20};
	std::vector<Agent> agents;
	agents.emplace_back(track, track.pointAt(-6), -6);
	agents.emplace_back(track, track.pointAt(-12), -12);
	agents[0].setNetwork(constantControls(0, 1));
	agents[1].setNetwork(constantControls(0, 1));
	REQUIRE(selectTrainingCameraTarget(agents) == 0);
	advance(agents[0], 400);
	REQUIRE_FALSE(agents[0].isActive());
	REQUIRE(agents[1].isActive());
	REQUIRE(selectTrainingCameraTarget(agents, 0) == 1);
	advance(agents[1], 400);
	REQUIRE_FALSE(agents[1].isActive());
	REQUIRE(selectTrainingCameraTarget(agents, 1) == 1);
	REQUIRE(selectTrainingCameraTarget(agents) == 0);
}

TEST_CASE("La camera conserve la cible actuelle a egalite et ignore les indices invalides", "[camera][training]") {
	const Track track{800, 500, 20};
	std::vector<Agent> agents;
	agents.emplace_back(track, track.pointAt(-6), -6);
	agents.emplace_back(track, track.pointAt(-6), -6);
	REQUIRE(selectTrainingCameraTarget(agents) == 0);
	REQUIRE(selectTrainingCameraTarget(agents, 1) == 1);
	REQUIRE(selectTrainingCameraTarget(agents, 99) == 0);
	REQUIRE_FALSE(selectTrainingCameraTarget({}, 1).has_value());
}

TEST_CASE("La position de course conserve le decalage de grille apres une remise a zero", "[camera][training]") {
	const Track track{40, 30, 10};
	Agent agent{track, track.pointAt(-120), -120};
	REQUIRE(agent.racePosition() == -120);
	REQUIRE(agent.progress() == 0);
	agent.setNetwork(constantControls(1));
	advance(agent, 30);
	REQUIRE(agent.progress() > 0);
	REQUIRE(agent.racePosition() == Catch::Approx(-120 + agent.progress()));
	agent.reset();
	REQUIRE(agent.racePosition() == -120);
	REQUIRE(agent.progress() == 0);
	REQUIRE(agent.bestProgress() == 0);
	REQUIRE(agent.isActive());
}

TEST_CASE("La position de course par defaut se repere autour de la ligne de depart", "[camera][training]") {
	const Track track{800, 500, 20};
	const Agent before{track, track.pointAt(-6)};
	const Agent after{track, track.pointAt(6)};
	REQUIRE(before.racePosition() == Catch::Approx(-6));
	REQUIRE(after.racePosition() == Catch::Approx(6));
}
