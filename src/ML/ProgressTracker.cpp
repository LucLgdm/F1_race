/* ****************************************** */
/*   ProgressTracker.cpp                      */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/16 14:20:35 by Luc      */
/*   Updated: 2026/10/16 14:20:35 by Luc      */
/* ****************************************** */

#include "ML/ProgressTracker.hpp"

ProgressTracker::ProgressTracker(float trackLength, float initialCoordinate):
		_trackLength(trackLength), _previousCoordinate(initialCoordinate) {
	if (!std::isfinite(trackLength) || trackLength <= 0.0f)
		throw neuralError("TrackLength must be finite and positive");
}

/************************************
 * Choose the shoortest move signed
 * while counting the lap by zero
 ************************************/
void ProgressTracker::update(float coordinate, float dt) {
	if (!std::isfinite(dt) || dt <= 0.0f)
		throw neuralError("Progress ti;estep must be finite or positive");
	
	const float delta = std::remainder(coordinate - _previousCoordinate, _trackLength);
	_previousCoordinate = coordinate;
	_progress += delta;
	_bestProgress = std::max(_bestProgress, _progress);
	_timeWithoutProgress += dt;

	if (_bestProgress - _progressAtLastReset >= _minimumProgress) {
		_progressAtLastReset = _bestProgress;
		_timeWithoutProgress = 0.0f;
	}
}

void ProgressTracker::reset(float initialCoordinate) {
	_previousCoordinate = initialCoordinate;
	_progress = 0.0f;
	_bestProgress = 0.0f;

	_timeWithoutProgress = 0.0f;
	_progressAtLastReset = 0.0f;
}

float ProgressTracker::progress() const noexcept {
	return _progress;
}

float ProgressTracker::bestProgress() const noexcept {
	return _bestProgress;
}

float ProgressTracker::timeWithoutProgress() const noexcept {
	return _timeWithoutProgress;
}

bool ProgressTracker::hasStalled(float timeout) const noexcept {
	return _timeWithoutProgress >= timeout;
}
