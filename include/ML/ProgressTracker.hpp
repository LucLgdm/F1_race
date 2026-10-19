/* ****************************************** */
/*   ProgressTracker.hpp                      */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/16 14:15:17 by Luc      */
/*   Updated: 2026/10/16 14:15:17 by Luc      */
/* ****************************************** */

#pragma once

#include <algorithm>
#include <cmath>
#include "Exception.hpp"

class ProgressTracker {
	public:
		ProgressTracker(float trackLength, float initialCoordinate);

		void update(float coordinate, float dt);
		void reset(float initialCoordinate);
		[[nodiscard]] float progress() const noexcept;
		[[nodiscard]] float bestProgress() const noexcept;
		[[nodiscard]] float timeWithoutProgress() const noexcept;
		[[nodiscard]] bool hasStalled(float timeout) const noexcept;


	private:
		float _trackLength;
		float _previousCoordinate;

		float _progress = 0.0f;
		float _bestProgress = 0.0f;
		float _timeWithoutProgress = 0.0f;
		float _progressAtLastReset = 0.0f;

		static constexpr float _minimumProgress = 0.5f;
};