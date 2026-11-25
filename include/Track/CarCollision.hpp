#pragma once
#include <array>
#include <optional>
#include "Track/Car.hpp"
#include "Track/VehicleGeometry.hpp"

// Plain snapshots, independent of renderer, trainer, RNG and ownership.
struct CarSnapshot {
    Vec2 position{};
    float heading = 0.0f;
    Vec2 velocity{};
};
struct CarFootprint {
    float halfLength = VehicleGeometry::collisionHalfLength;
    float halfWidth = VehicleGeometry::collisionHalfWidth;
};
[[nodiscard]] CarSnapshot snapshotCar(const Car& car) noexcept;
[[nodiscard]] std::array<Vec2, 4> footprintCorners(const CarSnapshot& car, CarFootprint shape = {});
[[nodiscard]] bool carsOverlap(const CarSnapshot& a, const CarSnapshot& b, CarFootprint shape = {});
// Continuous conservative advancement of linearly interpolated poses, including rotation.
// Returns a contact fraction in [0,1]. A <= 0.1 mm tolerance is used, not a safety halo.
[[nodiscard]] std::optional<float> carContact(const CarSnapshot& a0, const CarSnapshot& a1,
    const CarSnapshot& b0, const CarSnapshot& b1, CarFootprint shape = {});
