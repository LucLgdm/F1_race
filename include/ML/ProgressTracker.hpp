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

		void update(float coordinate);
		void reset(float initialCoordinate);
		[[nodiscard]] float progress() const noexcept;
		[[nodiscard]] float bestProgress() const noexcept;


	private:
		float _trackLength;
		float _previousCoordinate;

		float _progress = 0.0f;
		float _bestProgress = 0.0f;
};