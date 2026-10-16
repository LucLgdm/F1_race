/* ****************************************** */
/*   Agent.hpp                                */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/16 11:04:51 by Luc      */
/*   Updated: 2026/10/16 11:04:51 by Luc      */
/* ****************************************** */

#pragma once

#include <algorithm>
#include <vector>

#include "Exception.hpp"
#include "Simulation.hpp"
#include "ML/Network.hpp"


class Agent {
	public:
		explicit Agent(const Track& track);

		void step(float dt);
		void reset();

		[[nodiscard]] const Car& car() const noexcept;
		[[nodiscard]] bool isActive() const noexcept;

	private:
		Simulation _simulation;
		Network _network;

		bool _active = true;
		float _elapsedTime = 0.0f;

		static constexpr float maxDuration = 20.0f;
};