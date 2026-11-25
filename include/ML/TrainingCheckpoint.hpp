#pragma once
#include <filesystem>
#include <limits>
#include <optional>
#include "ML/DrivingMode.hpp"
#include "Track/ChampionIO.hpp"

struct TrainingCheckpoint {
    std::filesystem::path destination;
    std::filesystem::path origin;
    std::optional<Network> record;
    std::optional<Network> initial;
    float recordTime = std::numeric_limits<float>::infinity();
    bool transferredChrono = false;
};

// Read-only preparation. A fresh explicit chrono seed always wins over old race
// checkpoints and chooses an unused destination, preserving both existing files.
[[nodiscard]] TrainingCheckpoint prepareTrainingCheckpoint(const Track& track,
    const std::filesystem::path& trackFile, DrivingMode mode, const Network* seed = nullptr,
    const std::filesystem::path& seedFile = {}, bool freshFromChrono = false);
