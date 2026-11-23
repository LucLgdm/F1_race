#include "ML/Agent.hpp"
#include "Track/ChampionIO.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) try {
	if (argc != 2) {
		std::cerr << "Usage: F1_training_benchmark champion.f1\n";
		return 1;
	}
	std::ifstream input{argv[1]};
	const auto champion = ChampionIO::read(input);
	std::array<double, 3> times;
	for (auto& elapsed : times) {
		std::vector<Agent> agents;
		agents.reserve(20);
		std::mt19937 generator{20261123};
		for (int i = 0; i < 20; ++i) {
			const float coordinate = -6.0f - i * 6.0f;
			const Vec2 center = champion.track.pointAt(coordinate);
			const float heading = champion.track.headingAt(coordinate);
			const float lateral = std::min(3.0f, champion.track.roadWidth() / 2 - 2.3f - 0.25f);
			if (lateral <= 0)
				throw trackError("Track too narrow for the starting grid");
			const float offset = i % 2 == 0 ? lateral : -lateral;
			agents.emplace_back(champion.track, Vec2{center.x - std::sin(heading) * offset,
													  center.y + std::cos(heading) * offset});
			agents.back().setNetwork(champion.network);
			if (i > 0)
				agents.back().mutateNetwork(i <= 6 ? 0.05f : i <= 12 ? 0.1f : 0.2f,
										   i <= 6 ? 0.03f : i <= 12 ? 0.1f : 0.3f, generator);
		}
		const auto start = std::chrono::steady_clock::now();
		std::size_t steps = 0;
		int ticks = 0;
		bool active;
		do {
			active = false;
			for (auto& agent : agents) {
				if (agent.isActive()) {
					agent.step(1.0f / 120.0f);
					++steps;
				}
				active = active || agent.isActive();
			}
			++ticks;
		} while (active && ticks < 1000000);
		elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
		std::cout << std::setprecision(17) << "seconds=" << elapsed << " ticks=" << ticks
			<< " agent_steps=" << steps << " triangles=" << champion.track.mesh().size() << '\n';
		for (std::size_t i = 0; i < agents.size(); ++i) {
			const auto& a = agents[i];
			std::cout << "agent=" << i << " progress=" << a.bestProgress() << " laps=" << a.lapCount()
				<< " time=" << a.lastLapTime() << " x=" << a.car().position().x << " y=" << a.car().position().y << '\n';
		}
		if (active)
			return 2;
	}
	std::sort(times.begin(), times.end());
	std::cout << "median_seconds=" << times[1] << '\n';
} catch (const std::exception& error) {
	std::cerr << error.what() << '\n';
	return 1;
}
