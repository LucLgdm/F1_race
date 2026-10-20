/* ****************************************** */
/*   LapCounter.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/20 11:02:24 by Luc      */
/*   Updated: 2026/10/20 11:02:24 by Luc      */
/* ****************************************** */

#pragma once

#include <cmath>
#include <cstddef>

#include "Exception.hpp"

class LapCounter {
	public:
		LapCounter(float trackLength, float initialCoordinate);
		
		void reset(float coordinate);
		bool update(float coordinate, float dt);
		
		[[nodiscard]] std::size_t lapCount() const noexcept;
		[[nodiscard]] float currentLapTime() const noexcept;
		[[nodiscard]] float lastLapTime() const noexcept;

	private:
		float _trackLength;
		float _previousCoordinate = 0.0f;

		float _elapsedTime = 0.0f;
		float _lapStartTime = 0.0f;
		float _lastLapTime = 0.0f;

		std::size_t _lapCount = 0;
		int _nextCheckpoint = 0;
		bool _started = false;

};