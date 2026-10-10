/* ****************************************** */
/*   Car.hpp                         */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:09:52 by Luc      */
/*   Updated: 2026/10/05 15:09:52 by Luc      */
/* ****************************************** */

#pragma once

#include <algorithm>
#include <cmath>

struct Vec2 {
	float x = 0.0f;
	float y = 0.0f;
};

struct CarControls {
	float throttle = 0.0f; // [0, 1]
	float brake = 0.0f;    // [0, 1]
	float steering = 0.0f; // [-1, 1] : left, right
};

// explicit 	 : provides an implicit conversion from Vec2 to Car
// [[nodiscard]] : Ignoring the returned value is probalby an error
// noexcept		 : the operation doesn't send an exception
class Car {
	public:
		explicit Car(Vec2 position = {}) noexcept;

		void update(const CarControls& controls, float deltaTime);

		// Getters
		[[nodiscard]] Vec2 position() const noexcept;
		[[nodiscard]] float heading() const noexcept;
		[[nodiscard]] float speed() const noexcept;
		[[nodiscard]] float steeringAngle() const noexcept;

	private:
		Vec2 _position;
		float _heading = 0.0f; // Radians
		float _speed = 0.0f;   // Meter per sec
		float _steeringAngle = 0.0f; // Radians, positive on the left
};
