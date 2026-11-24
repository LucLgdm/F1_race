#include "Track/TrackIO.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

int main(int argc, char** argv) try {
	if (argc != 2) {
		std::cerr << "Usage: F1_track_benchmark circuit.track\n";
		return 1;
	}
	const Track track = TrackIO::load(argv[1]);
	struct Query { Vec2 from, to; float heading, coordinate; };
	std::vector<Query> queries;
	for (int i = 0; i < 5120; ++i) {
		const float coordinate = track.length() * (float(i % 1024) + 0.25f) / 1024;
		const float heading = track.headingAt(coordinate);
		const auto center = track.pointAt(coordinate);
		const float lateral = std::min(2.0f, std::max(0.0f, track.roadWidth() / 2 - 2.4f));
		const float offset = (i % 2 == 0) ? lateral : -lateral;
		const Vec2 from{center.x - std::sin(heading) * offset, center.y + std::cos(heading) * offset};
		queries.push_back({from, {from.x + std::cos(heading) * 0.12f,
								 from.y + std::sin(heading) * 0.12f}, heading, coordinate});
	}
	std::array<double, 5> times;
	double checksum = 0;
	for (auto& elapsed : times) {
		const auto start = std::chrono::steady_clock::now();
		double result = 0;
		for (const auto& query : queries) {
			for (float angle : {-0.5f, -0.25f, 0.0f, 0.25f, 0.5f})
				result += track.raycast(query.from, query.heading + angle * std::numbers::pi_v<float>, 40);
			result += track.canTravel(query.from, query.to, 2.3f);
			result += track.locate(query.to, query.coordinate, 3).coordinate;
		}
		elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		checksum = result;
	}
	std::sort(times.begin(), times.end());
	std::cout << std::setprecision(17) << "triangles=" << track.mesh().size()
		<< " edges=" << track.boundaries().size() << " batches=" << queries.size()
		<< " median_ms=" << times[2] << " checksum=" << checksum << '\n';
} catch (const std::exception& error) {
	std::cerr << error.what() << '\n';
	return 1;
}
