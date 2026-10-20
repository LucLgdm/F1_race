/* ****************************************** */
/*   LapCounter.cpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/20 11:23:03 by Luc      */
/*   Updated: 2026/10/20 11:23:03 by Luc      */
/* ****************************************** */

#include "Track/LapCounter.hpp"

LapCounter::LapCounter(float trackLength, float initialCoordinate): _trackLength(trackLength) {
	if (!std::isfinite(trackLength) || trackLength <= 0.0f)
		throw neuralError("Invalide track length");

	reset(initialCoordinate);
}

void LapCounter::reset(float coordinate) {
	_previousCoordinate = coordinate;

	_elapsedTime = 0.0f;
	_lapStartTime = 0.0f;
	_lastLapTime = 0.0f;
	_lapCount = 0;

	// if the car is on the startline, the first lap starts immediatly
	_started = (coordinate == 0.0f);
	_nextCheckpoint = _started ? 1 : 0;
}

bool LapCounter::update(float coordinate, float dt) {
	if (!std::isfinite(dt) || dt <= 0.0f)
		throw neuralError("Invalid lap timestep");

	const float previous = _previousCoordinate;
	const float delta = std::remainder(coordinate - previous, _trackLength);

	_previousCoordinate = coordinate;
	_elapsedTime += dt;

	// Move in wrong way doesn't validate anything
	if (delta <= 0.0f)
		return false;

	 // Points : 0, L/4, L/2, 3L/4.
	const float target = _nextCheckpoint * (_trackLength / 4.0f);

	const float distance = std::fmod(target - previous + _trackLength, _trackLength);

	if (distance <= 0.0f || distance > delta)
		return false;

	// Estimation of the time when crossing the line
	const float crossingTime = _elapsedTime - dt + dt * (distance / delta);

	bool finished = false;

	if (_nextCheckpoint == 0) {
		if (_started) {
			_lastLapTime = crossingTime - _lapStartTime;
			++_lapCount;
			finished = true;
		}

		_started = true;
		_lapStartTime = crossingTime;
	}

	_nextCheckpoint = (_nextCheckpoint + 1) % 4;
	return finished;
}

std::size_t LapCounter::lapCount() const noexcept {
	return _lapCount;
}

float LapCounter::currentLapTime() const noexcept {
	return _started ? _elapsedTime - _lapStartTime : 0.0f;
}

float LapCounter::lastLapTime() const noexcept {
	return _lastLapTime;
}