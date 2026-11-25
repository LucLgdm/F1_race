#pragma once
#include <array>
#include <memory>
#include <optional>
#include <vector>
#include "ML/Network.hpp"
#include "ML/ProgressTracker.hpp"
#include "ML/RaceObservation.hpp"
#include "Simulation.hpp"

// Evaluation v1. Change the saved EVALUATION version when changing this suite.
struct RaceScenario {
    float start = 0.0f;
    bool traffic = true;
    float gap = 18.0f;
    float opponentSpeed = 8.0f;
    float lateralOffset = 0.0f;
    float soloDistance = 100.0f;
};
[[nodiscard]] std::array<RaceScenario,5> raceScenarios(const Track& track);

enum class RaceEnd { Running, Success, CarContact, WallContact, Timeout, Stalled };
struct RaceScore {
    std::size_t successes = 0;
    std::size_t carContacts = 0;
    std::size_t wallContacts = 0;
    float progress = 0.0f; // sum of capped, normalized scenario progress
    float time = 0.0f;
};
[[nodiscard]] bool betterRaceScore(const RaceScore& a, const RaceScore& b);

class RaceEpisode {
public:
    RaceEpisode(const Track& track, RaceScenario scenario);
    [[nodiscard]] CarControls decide(const Network& network) const;
    void advance(const CarControls& controls, float dt);
    [[nodiscard]] const Car& car() const { return _simulation.car(); }
    [[nodiscard]] std::optional<CarSnapshot> opponent() const;
    [[nodiscard]] RaceEnd end() const { return _end; }
    [[nodiscard]] float elapsed() const { return _elapsed; }
    [[nodiscard]] float progress() const;
private:
    const Track& _track;
    RaceScenario _scenario;
    Simulation _simulation;
    ProgressTracker _progress;
    float _elapsed = 0.0f;
    float _aheadTime = 0.0f;
    RaceEnd _end = RaceEnd::Running;
    [[nodiscard]] CarSnapshot opponentAt(float time) const;
};

// Independent candidate worlds: every policy sees the same five scenarios.
// Only episode state is writable during evaluation. RNG/selection occur at generation barriers.
// All decisions are computed before any episode advances. No renderer or shared RNG in workers.
class RaceTraining {
public:
    RaceTraining(const Track& track, const Network* seed = nullptr, std::size_t population = 20);
    void step(float dt);
    [[nodiscard]] bool finished() const;
    [[nodiscard]] std::size_t bestIndex() const;
    [[nodiscard]] std::size_t activeCount() const;
    [[nodiscard]] const RaceEpisode& episode(std::size_t i) const { return *_episodes.at(i); }
    [[nodiscard]] const Network& network(std::size_t i) const { return _networks.at(i); }
    [[nodiscard]] const RaceScore& score(std::size_t i) const { return _scores.at(i); }
    [[nodiscard]] std::size_t scenarioIndex(std::size_t i) const { return _scenarioIndices.at(i); }
    [[nodiscard]] std::size_t size() const { return _networks.size(); }
    [[nodiscard]] std::size_t scenarioCount() const { return _scenarios.size(); }
    void evolve(std::mt19937& rng);
private:
    const Track& _track;
    std::array<RaceScenario,5> _scenarios;
    std::vector<Network> _networks;
    std::vector<std::unique_ptr<RaceEpisode>> _episodes;
    std::vector<std::size_t> _scenarioIndices;
    std::vector<RaceScore> _scores;
    std::vector<CarControls> _controls;
    void resetEpisodes();
};
