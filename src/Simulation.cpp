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

Simulation::Simulation(const Track& track, Vec2 startPosition): _track(track),
				_startPosition(startPosition), _car(startPosition), _lapCounter(track.length(), track.progressAt(startPosition)) {
	if (!_track.canDrive(startPosition, _car.collisionRadius()))
		throw trackError("Invalid starting position");
}

bool Simulation::step(const CarControls& controls, float dt) {
	const Car previousState = _car;
	_car.update(controls, dt);

	bool collided = !_track.canDrive(_car.position(), _car.collisionRadius());
	if (collided) {
		_car = previousState;
		_car.stop();
	}

	_trackFinished = _lapCounter.update(_track.progressAt(_car.position()), dt);
	_lapCount = _lapCounter.lapCount();
	_timePerTrack = _lapCounter.currentLapTime();
	_lastLapTime = _lapCounter.lastLapTime();
	
	return collided;
}

void Simulation::reset() {
	_car = Car{_startPosition};

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
	return observeCar(_car, _track, sensorRange);
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