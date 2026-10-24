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
#include <cmath>

#include "Track/Car.hpp"
#include "Track/Track.hpp"
#include "Track/CarObservation.hpp"

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

[[nodiscard]] inline CarObservation observeCar(const Car& car, const Track& track, float sensorRange, const TrackLocation* location = nullptr) {
	CarObservation observation;
	observation.wallDistance = measureWallDistances(car, track, sensorRange);
	observation.speed = car.speed();

	const float trackHeading = location ? location->heading : track.referenceHeading(car.position());
	observation.headingError = std::remainder(car.heading() - trackHeading, 2.0f * std::numbers::pi_v<float>);

	return observation;
}
