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
#include <optional>
#include <string>
#include "ML/DrivingMode.hpp"
#include "ML/RaceSession.hpp"

struct TrainingSettings {
    DrivingMode mode = DrivingMode::Chrono;
    bool fullRace = false;
    bool restartRace = false;
    int raceCars = 8;
    int raceLaps = 1;
	bool paused = true;
	bool showSimulation = true;
	bool accelerated = false;
	bool trackOverview = false;
};

struct RaceStanding {
    std::size_t id = 0;
    std::size_t laps = 0;
    DriverStatus status = DriverStatus::Racing;
    float finishTime = 0.0f;
};

inline std::array<float,3> raceCarColor(std::size_t id) {
    constexpr std::array<std::array<float,3>,8> colors{{
        {1.0f,.25f,.2f},{.2f,.6f,1.0f},{1.0f,.85f,.15f},{.3f,.9f,.4f},
        {.8f,.35f,1.0f},{1.0f,.55f,.15f},{.2f,.9f,.9f},{1.0f,.5f,.75f}}};
    return colors[id%colors.size()];
}
inline const char* driverStatusText(DriverStatus status) {
    switch (status) {
        case DriverStatus::Racing: return "En course";
        case DriverStatus::Finished: return "Arrivee";
        case DriverStatus::CarContact: return "Contact";
        case DriverStatus::WallContact: return "Sortie";
        case DriverStatus::Stalled: return "Immobilise";
        case DriverStatus::Timeout: return "Temps limite";
    }
    return "";
}

struct TrainingStats {
	std::size_t generation = 1;
	std::size_t activeAgents = 0;
	std::size_t populationSize = 0;

	bool hasChampion = false;
	float bestLapTime = 0.0f;
    bool qualifiedStart = false;
    std::optional<std::size_t> raceSuccesses;
    std::size_t carContacts = 0;
    std::size_t wallContacts = 0;
    std::size_t raceScenario = 0;
    float raceElapsed = 0.0f;
    bool raceFinished = false;
    std::size_t raceLapTarget = 1;
    std::vector<RaceStanding> standings;
    std::string networkOrigin;
    std::string checkpointPath;
    std::vector<std::size_t> networkShape;
    bool transferredChrono = false;
    float speedKmh = 0.0f;
    bool hasSlowOpponent = false;
    float opponentSpeedKmh = 0.0f;
    CarControls controls{};
};


enum class ChampionLoadMode {
	ResumeTraining,
	Replay,
    SeedRace,
    StartFullRace
};

struct ChampionLoadRequest {
	std::string path;
	ChampionLoadMode mode;
};