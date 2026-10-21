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

struct TrainingSettings {
	bool paused = true;
	bool showSimulation = true;
	bool accelerated = false;
};

struct TrainingStats {
	std::size_t generation = 1;
	std::size_t activeAgents = 0;
	std::size_t populationSize = 0;

	bool hasChampion = false;
	float bestLapTime = 0.0f;
};


enum class ChampionLoadMode {
	ResumeTraining,
	Replay
};

struct ChampionLoadRequest {
	std::string path;
	ChampionLoadMode mode;
};