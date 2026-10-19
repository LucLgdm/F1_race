/* ****************************************** */
/*   Agent.cpp                                */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/16 11:08:04 by Luc      */
/*   Updated: 2026/10/16 11:08:04 by Luc      */
/* ****************************************** */

#include "ML/Agent.hpp"
#include <iostream>

Agent::Agent(const Track& track, Vec2 startPosition): _track(track), _simulation(track, startPosition),
				_network(std::vector<Layer>{Layer{8 , 16, Activation::Tanh},
											Layer{16, 16, Activation::Tanh},
											Layer{16, 3, Activation::Tanh}}),
			_progress(track.length(), track.progressAt(startPosition)) {}

void Agent::step(float dt) {
	if (!_active)
		return;

	const auto observation = _simulation.observe();
	const auto inputs = observation.normalized(40.0f, 50.0f);

	const auto outputs = _network.forward(inputs);
	if (outputs.size() != 3)
		throw neuralError("A driving network must produce three outputs");

	CarControls controls;
	controls.steering = std::clamp(outputs[0], -1.0f, 1.0f);
	controls.throttle = std::clamp(outputs[1], 0.0f, 1.0f);
	controls.brake    = std::clamp(outputs[2], 0.0f, 1.0f);

	const bool collided = _simulation.step(controls, dt);
	_progress.update(_track.progressAt(_simulation.car().position()), dt);
	_elapsedTime += dt;
	if (collided || _elapsedTime >= _maxDuration || _progress.hasStalled(_maxTimeWithoutProgress))
		_active = false;
}

const Car& Agent::car() const noexcept {
	return _simulation.car();
}

bool Agent::isActive() const noexcept {
	return _active;
}

float Agent::progress() const noexcept {
	return _progress.progress();
}

float Agent::bestProgress() const noexcept {
	return _progress.bestProgress();
}

const Network& Agent::network() const noexcept {
	return _network;
}

void Agent::setNetwork(const Network& network) {
	_network = network;
}

void Agent::reset() {
	_simulation.reset();
	_progress.reset(_track.progressAt(_simulation.car().position()));
	_active = true;
	_elapsedTime = 0.0f;
}

void Agent::mutateNetwork(float probability, float strength, std::mt19937& generator) {
	_network.mutate(probability, strength, generator);
}

void Agent::randomizeNetwork() {
	_network = Network{
		std::vector<Layer> {
			Layer{8, 16, Activation::Tanh},
			Layer{16, 16, Activation::Tanh},
			Layer{16, 3, Activation::Tanh}
		}
	};
}