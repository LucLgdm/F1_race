/* ****************************************** */
/*   Simulation.cpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/15 14:45:21 by Luc      */
/*   Updated: 2026/10/15 14:45:21 by Luc      */
/* ****************************************** */

#include "Simulation.hpp"

Simulation::Simulation(const Track& track): _track(track), _car(track.startPosition()) {}

Simulation::Simulation(const Track& track, Vec2 startPosition): _track(track), _startPosition(startPosition), _car(startPosition) {
	if (!_track.canDrive(startPosition, _car.collisionRadius()))
		throw trackError("Invalid starting position");
}

bool Simulation::step(const CarControls& controls, float dt) {
	const Car previousState = _car;
	_car.update(controls, dt);

	if (!_track.canDrive(_car.position(), _car.collisionRadius())) {
		_car = previousState;
		_car.stop();
		return true;
	}
	return false;
}

void Simulation::reset() {
	_car = Car{_startPosition};
}

const Car& Simulation::car() const noexcept {
	return _car;
}

CarObservation Simulation::observe() const {
	return observeCar(_car, _track, sensorRange);
}
