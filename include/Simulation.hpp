/* ****************************************** */
/*   Simulation.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/15 14:40:10 by Luc      */
/*   Updated: 2026/10/15 14:40:10 by Luc      */
/* ****************************************** */

#pragma once

#include "Car.hpp"
#include "Track.hpp"
#include "CarSensors.hpp"
#include "CarObservation.hpp"

class Simulation {
	public:
		explicit Simulation(const Track& track);
		Simulation(const Track& track, Vec2 startPosition);

		// return true if a collision happend
		bool step(const CarControls& controls, float dt);

		void reset();
		[[nodiscard]] const Car& car() const noexcept;
		[[nodiscard]] CarObservation observe() const;

	private:
		const Track& _track;
		Vec2 _startPosition;
		Car _car;

		static constexpr float sensorRange = 40.0f;
};
