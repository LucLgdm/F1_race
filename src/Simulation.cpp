/* ****************************************** */
/*   Simulation.cpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/15 14:45:21 by Luc      */
/*   Updated: 2026/10/15 14:45:21 by Luc      */
/* ****************************************** */

#include "Simulation.hpp"

// Simulation::Simulation(const Track& track): _track(track), _car(track.startPosition()) {}

Simulation::Simulation(const Track& track, Vec2 startPosition)
	: _track(track), _startPosition(startPosition),
	  _car(startPosition, track.referenceHeading(startPosition)), _location(track.locate(startPosition)),
	  _lapCounter(track.length(), track.progressAt(startPosition)) {
	if (!_track.canDrive(startPosition, _car.collisionRadius()))
		throw trackError("Invalid starting position");
}

bool Simulation::step(const CarControls& controls, float dt) {
	if (!std::isfinite(dt) || dt <= 0.0f || dt > 1.0f)
		throw carError("Simulation timestep must be in (0, 1]");
	bool collided = false;
	_trackFinished = false;
	const int steps = std::max(1, static_cast<int>(std::ceil(dt * 120.0f)));
	const float step = dt / steps;
	for (int i = 0; i < steps; ++i) {
		const Car previousState = _car;
		_car.update(controls, step);
		if (!_track.canTravel(previousState.position(), _car.position(), _car.collisionRadius())) {
			_car = previousState;
			_car.stop();
			collided = true;
		}
		const float maxTravel =
			std::max(_track.roadWidth() * 1.5f, previousState.speed() * step * 3.0f + 1.0f);
		_location = _track.locate(_car.position(), _location.coordinate, maxTravel);
		_trackFinished = _lapCounter.update(_location.coordinate, step) || _trackFinished;
	}

	_lapCount = _lapCounter.lapCount();
	_timePerTrack = _lapCounter.currentLapTime();
	_lastLapTime = _lapCounter.lastLapTime();

	return collided;
}

void Simulation::reset() {
	_car = Car{_startPosition, _track.referenceHeading(_startPosition)};
	_location = _track.locate(_startPosition);

	_lapCounter.reset(_track.progressAt(_startPosition));

	_lapCount = 0;
	_timePerTrack = 0.0f;
	_lastLapTime = 0.0f;
	_trackFinished = false;
}

const Car& Simulation::car() const noexcept {
	return _car;
}

CarObservation Simulation::observe() const {
	return observeCar(_car, _track, sensorRange, &_location);
}

std::size_t Simulation::lapCount() const noexcept {
	return _lapCount;
}

float Simulation::timePerTrack() const noexcept {
	return _timePerTrack;
}

float Simulation::lastLapTime() const noexcept {
	return _lastLapTime;
}

bool Simulation::trackFinished() const noexcept {
	return _trackFinished;
}