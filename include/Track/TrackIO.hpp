#pragma once
#include <filesystem>
#include <iosfwd>
#include <string>
#include "Track/Track.hpp"

class TrackIO {
  public:
	static void write(std::ostream& output, const Track& track);
	// Reads exactly one track, also usable inside a champion file.
	[[nodiscard]] static Track read(std::istream& input);
	static void save(const std::filesystem::path& path, const Track& track);
	[[nodiscard]] static Track load(const std::filesystem::path& path);
	static void saveText(const std::filesystem::path& path, const std::string& text);
};
