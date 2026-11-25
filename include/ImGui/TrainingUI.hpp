/* ****************************************** */
/*   TrainingUI.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/21 11:29:13 by Luc      */
/*   Updated: 2026/10/21 11:29:13 by Luc      */
/* ****************************************** */

#pragma once

#include <cstddef>
#include <string>
#include "ML/DrivingMode.hpp"

struct TrainingSettings {
    DrivingMode mode = DrivingMode::Chrono;
	bool paused = true;
	bool showSimulation = true;
	bool accelerated = false;
	bool trackOverview = false;
};

struct TrainingStats {
	std::size_t generation = 1;
	std::size_t activeAgents = 0;
	std::size_t populationSize = 0;

	bool hasChampion = false;
	float bestLapTime = 0.0f;
    std::size_t raceSuccesses = 0;
    std::size_t carContacts = 0;
    std::size_t wallContacts = 0;
    std::size_t raceScenario = 0;
};


enum class ChampionLoadMode {
	ResumeTraining,
	Replay,
    SeedRace
};

struct ChampionLoadRequest {
	std::string path;
	ChampionLoadMode mode;
};