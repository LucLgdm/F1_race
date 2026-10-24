/* ****************************************** */
/*   Simulation.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/15 14:40:10 by Luc      */
/*   Updated: 2026/10/15 14:40:10 by Luc      */
/* ****************************************** */

#pragma once

#include "Track/Car.hpp"
#include "Track/Track.hpp"
#include "Track/CarSensors.hpp"
#include "Track/CarObservation.hpp"
#include "Track/LapCounter.hpp"

class Simulation {
	public:
		// explicit Simulation(const Track& track);
		Simulation(const Track& track, Vec2 startPosition);

		// return true if a collision happend
		bool step(const CarControls& controls, float dt);

		void reset();
		[[nodiscard]] const TrackLocation& location() const noexcept { return _location; }
		[[nodiscard]] const Car& car() const noexcept;
		[[nodiscard]] CarObservation observe() const;
		[[nodiscard]] std::size_t lapCount() const noexcept;
		[[nodiscard]] float timePerTrack() const noexcept;
		[[nodiscard]] float lastLapTime() const noexcept;
		[[nodiscard]] bool trackFinished() const noexcept;

	private:
		const Track& _track;
		Vec2 _startPosition;
		Car _car;
		TrackLocation _location;

		LapCounter _lapCounter;

		std::size_t _lapCount = 0;
		float _timePerTrack = 0.0f;
		float _lastLapTime = 0.0f;
		bool _trackFinished = false;

		static constexpr float sensorRange = 40.0f;
};
