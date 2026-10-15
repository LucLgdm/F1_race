/* ****************************************** */
/*   CarObservation.hpp                       */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/14 16:24:07 by Luc      */
/*   Updated: 2026/10/14 16:24:07 by Luc      */
/* ****************************************** */

#pragma once

#include <array>
#include <cmath>
#include "Exception.hpp"

struct CarObservation {
	std::array<float, 5> wallDistance{};

	float speed = 0.0f;
	float headingError = 0.0f;	// in radian

	[[nodiscard]] std::array<float, 8> normalized(float sensorRange, float maxSpeed) const {
		if (sensorRange <= 0.0f || maxSpeed <= 0.0f)
			throw carError("SensorRange and maxSpeed must be strict positive");

		return {wallDistance[0] / sensorRange,
				wallDistance[1] / sensorRange,
				wallDistance[2] / sensorRange,
				wallDistance[3] / sensorRange,
				wallDistance[4] / sensorRange,
				speed / maxSpeed,
				std::sin(headingError),
				std::cos(headingError)
		};
	}
};
