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
void ProgressTracker::update(float coordinate) {
	const float delta = std::remainder(coordinate - _previousCoordinate, _trackLength);
	_previousCoordinate = coordinate;
	_progress += delta;
	_bestProgress = std::max(_bestProgress, _progress);
}

void ProgressTracker::reset(float initialCoordinate) {
	_previousCoordinate = initialCoordinate;
	_progress = 0.0f;
	_bestProgress = 0.0f;
}

float ProgressTracker::progress() const noexcept {
	return _progress;
}

float ProgressTracker::bestProgress() const noexcept {
	return _bestProgress;
}