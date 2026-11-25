#pragma once

// Metres in vehicle-local coordinates. Rendering details can change independently.
// The footprint includes wheels at their maximum steering angle.
struct VehicleGeometry {
    static constexpr float bodyHalfLength = 2.0f;
    static constexpr float bodyHalfWidth = 1.0f;
    static constexpr float wheelLongitudinal = 1.2f;
    static constexpr float wheelLateral = 1.1f;
    static constexpr float wheelScale = 0.2f;
    static constexpr float collisionHalfLength = 2.0f;
    static constexpr float collisionHalfWidth = 1.5f;
};
