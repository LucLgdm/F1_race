#include "Track/TrackIO.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <cmath>

namespace {
void expect(std::istream& input, const char* word) {
	std::string token;
	if (!(input >> token) || token != word)
		throw IOError("Invalid track file structure");
}
float number(std::istream& input) {
	float value;
	if (!(input >> value) || !std::isfinite(value))
		throw IOError("Invalid track number");
	return value;
}
} // namespace
void TrackIO::write(std::ostream& output, const Track& track) {
	output.imbue(std::locale::classic());
	output << std::setprecision(std::numeric_limits<float>::max_digits10);
	output << "F1_TRACK 1\nROAD_WIDTH " << track.roadWidth() << "\nSTART_DISTANCE " << track.startDistance()
		   << "\nSEGMENTS " << track.segments().size() << '\n';
	for (const auto& s : track.segments()) {
		output << (s.rounded ? "CURVE " : "STRAIGHT ") << s.start.x << ' ' << s.start.y << ' ' << s.heading
			   << ' ';
		if (s.rounded)
			output << s.radius << ' ' << s.turn;
		else
			output << s.length;
		output << '\n';
	}
	output << "END_TRACK\n";
	if (!output)
		throw IOError("Cannot write track");
}
Track TrackIO::read(std::istream& input) {
	input.imbue(std::locale::classic());
	expect(input, "F1_TRACK");
	int version;
	if (!(input >> version) || version != 1)
		throw IOError("Unsupported track version");
	expect(input, "ROAD_WIDTH");
	const float width = number(input);
	expect(input, "START_DISTANCE");
	const float start = number(input);
	expect(input, "SEGMENTS");
	int count;
	if (!(input >> count) || count < 0 || count > 256)
		throw IOError("Invalid segment count");
	std::vector<TrackSegment> segments;
	for (int i = 0; i < count; ++i) {
		std::string type;
		if (!(input >> type) || (type != "STRAIGHT" && type != "CURVE"))
			throw IOError("Unknown segment type");
		TrackSegment s;
		s.start.x = number(input);
		s.start.y = number(input);
		s.heading = number(input);
		s.rounded = type == "CURVE";
		if (s.rounded) {
			s.radius = number(input);
			if (!(input >> s.turn) || (s.turn != 1 && s.turn != -1))
				throw IOError("Invalid curve direction");
		} else
			s.length = number(input);
		segments.push_back(s);
	}
	expect(input, "END_TRACK");
	try {
		return Track{std::move(segments), width, start};
	} catch (const trackError& e) {
		throw IOError(e.what());
	}
}
Track TrackIO::load(const std::filesystem::path& path) {
	std::ifstream input{path, std::ios::binary};
	if (!input)
		throw IOError("Cannot open track file");
	Track track = read(input);
	input >> std::ws;
	if (!input.eof())
		throw IOError("Unexpected data after track");
	return track;
}
void TrackIO::save(const std::filesystem::path& path, const Track& track) {
	std::ostringstream output;
	write(output, track);
	saveText(path, output.str());
}
void TrackIO::saveText(const std::filesystem::path& path, const std::string& text) {
	if (!path.parent_path().empty())
		std::filesystem::create_directories(path.parent_path());
	const std::string suffix =
		"." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
	const std::filesystem::path temporary = path.string() + suffix + ".tmp",
								backup = path.string() + suffix + ".bak";
	bool moved = false;
	try {
		std::ofstream output{temporary, std::ios::binary | std::ios::trunc};
		output << text;
		output.close();
		if (!output)
			throw IOError("Cannot write circuit file");
		if (std::filesystem::exists(path)) {
			std::filesystem::rename(path, backup);
			moved = true;
		}
		std::filesystem::rename(temporary, path);
		if (moved) {
			std::error_code ec;
			std::filesystem::remove(backup, ec);
		}
	} catch (...) {
		std::error_code ec;
		if (moved && !std::filesystem::exists(path))
			std::filesystem::rename(backup, path, ec);
		std::filesystem::remove(temporary, ec);
		throw;
	}
}
