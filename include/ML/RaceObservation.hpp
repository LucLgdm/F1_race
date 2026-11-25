#pragma once
#include <array>
#include <span>
#include "Track/CarCollision.hpp"
#include "Track/CarObservation.hpp"

// Encoding v1: base 8, then 3 * [present, local x, local y, local vx, local vy].
// Nearest first; input order breaks exact distance ties deterministically.
[[nodiscard]] std::array<float,23> observeTraffic(const CarObservation& road,
    const CarSnapshot& self, std::span<const CarSnapshot> neighbours);
