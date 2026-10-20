/* ****************************************** */
/*   Car.cpp                         */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:46:38 by Luc      */
/*   Updated: 2026/10/05 15:46:38 by Luc      */
/* ****************************************** */

#include "Track/Car.hpp"

Car::Car(Vec2 position) noexcept : _position(position) {}

Vec2 Car::position() const noexcept {
	return _position;
}

float Car::heading() const noexcept {
	return _heading;
}

float Car::speed() const noexcept {
	return _speed;
}

float Car::steeringAngle() const noexcept {
	return _steeringAngle;
}

float Car::collisionRadius() const noexcept {
	return 2.3f;
}

void Car::stop() noexcept {
	_speed = 0.0f;
	_steeringAngle = 0.0f;
}

void Car::update(const CarControls& controls, float deltaTime) {
	constexpr float acceleration = 8.0f;  // m/s²
	constexpr float braking = 15.0f;      // m/s²
	constexpr float rollingDeceleration = 1.0f; // m/s²
	constexpr float maxSpeed = 50.0f;     // m/s, soit 180 km/h

	const float throttle = std::clamp(controls.throttle, 0.0f, 1.0f);
	const float brake = std::clamp(controls.brake, 0.0f, 1.0f);

	const float netAcceleration = throttle * acceleration - brake * braking - rollingDeceleration;

	// Speed calculation
	_speed += netAcceleration * deltaTime;
	_speed = std::clamp(_speed, 0.0f, maxSpeed);

	// Steering angle calculation
	constexpr float maxSteeringAngle = 0.45f; // Environ 26°
	constexpr float wheelbase = 2.4f;        // Distance entre les essieux

	// Nos commandes : -1 = gauche, +1 = droite.
	// Nos angles : positif = rotation vers la gauche.
	_steeringAngle = -std::clamp(controls.steering, -1.0f, 1.0f) * maxSteeringAngle;
	const float angularVelocity = (_speed / wheelbase) * std::tan(_steeringAngle);
	_heading += angularVelocity * deltaTime;

	// Position calculation
	_position.x += std::cos(_heading) * _speed * deltaTime;
	_position.y += std::sin(_heading) * _speed * deltaTime;
}