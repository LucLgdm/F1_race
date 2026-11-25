#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include "ML/Network.hpp"
#include "ML/ProgressTracker.hpp"
#include "ML/RaceObservation.hpp"
#include "Simulation.hpp"

struct RaceSettings {
    std::size_t cars = 8;
    std::size_t laps = 3;
    float maxDuration = 0.0f; // zero: computed from circuit length and lap target
    float stalledAfter = 10.0f;
};
struct RaceGridSlot {
    float coordinate = -6.0f;
    float lateral = 0.0f;
};
enum class DriverStatus { Racing, Finished, CarContact, WallContact, Stalled, Timeout };

// One shared world. Policies are read-only throughout a race; there is no evolution
// or record writing here. Prediction jobs write their own state, then a central
// collision barrier validates all motions before committing any of them.
class RaceSession {
public:
    RaceSession(const Track& track, const Network& policy, RaceSettings settings = {});
    RaceSession(const Track& track, std::vector<Network> policies, RaceSettings settings,
                std::vector<RaceGridSlot> grid = {});
    void step(float dt);
    // Same physics/collision pipeline with external commands (headless validation).
    void stepWithControls(std::span<const CarControls> controls, float dt);
    [[nodiscard]] std::size_t size() const { return _cars.size(); }
    [[nodiscard]] const Car& car(std::size_t i) const { return _cars.at(i).car(); }
    [[nodiscard]] const Network& policy(std::size_t i) const { return _policies.at(i); }
    [[nodiscard]] DriverStatus status(std::size_t i) const { return _status.at(i); }
    [[nodiscard]] bool onTrack(std::size_t i) const { return status(i)!=DriverStatus::Finished; }
    [[nodiscard]] std::size_t lapCount(std::size_t i) const { return _cars.at(i).lapCount(); }
    [[nodiscard]] float racePosition(std::size_t i) const;
    [[nodiscard]] float finishTime(std::size_t i) const { return _finishTimes.at(i); }
    [[nodiscard]] float elapsed() const { return _elapsed; }
    [[nodiscard]] std::size_t targetLaps() const { return _settings.laps; }
    [[nodiscard]] std::size_t contacts() const { return _contacts; }
    [[nodiscard]] std::size_t activeCount() const;
    [[nodiscard]] bool finished() const { return activeCount()==0; }
    [[nodiscard]] std::vector<std::size_t> standings() const;
    [[nodiscard]] std::optional<std::size_t> cameraTarget(std::optional<std::size_t> current = {}) const;
    [[nodiscard]] std::array<float,23> observation(std::size_t i) const;
private:
    const Track& _track;
    RaceSettings _settings;
    std::vector<Network> _policies;
    std::vector<Simulation> _cars, _predicted;
    std::vector<ProgressTracker> _progress;
    std::vector<RaceGridSlot> _grid;
    std::vector<DriverStatus> _status;
    std::vector<float> _finishTimes;
    std::vector<CarSnapshot> _before, _after;
    std::vector<std::array<float,23>> _inputs;
    std::vector<CarControls> _controls;
    std::vector<std::uint8_t> _blocked, _wall, _contact, _seenPairs;
    float _elapsed = 0.0f;
    std::size_t _contacts = 0;
    void advance(float dt, std::span<const CarControls> controls);
    void tick(float dt, std::span<const CarControls> controls);
};
