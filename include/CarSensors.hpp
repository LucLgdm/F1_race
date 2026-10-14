/* ****************************************** */
/*   CarSensors.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/14 17:04:19 by Luc      */
/*   Updated: 2026/10/14 17:04:19 by Luc      */
/* ****************************************** */

#pragma once

#include <array>
#include <numbers>

#include "Car.hpp"
#include "Track.hpp"

[[nodiscard]] inline std::array<float, 5> measureWallDistances(const Car& car, const Track& track, float sensorRange) {
	constexpr float pi = std::numbers::pi_v<float>;
	std::array<float, 5> offsets{pi / 2.0f, pi / 4.0f, 0.0f, -pi / 4.0f, -pi / 2.0f};

	std::array<float, 5> distances{};
	for (std::size_t i = 0; i < offsets.size(); ++i) {
		// car.heading() + offsets[i] allows the ray to turn with the car
		distances[i] = track.raycast(car.position(), car.heading() + offsets[i], sensorRange);
	}
	return distances;
}
