#pragma once
#include "Track/Track.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

struct TrackEditorRequest {
	Track track;
	std::string path;
	bool manual;
};

// Drafts are independent of the active simulation: no agent can retain stale
// progress or references while the user changes the circuit.
class TrackEditor {
  public:
	TrackEditor();
	void render();
	void setError(std::string message) { _message = std::move(message); }
	[[nodiscard]] const Track& track() const noexcept { return _track; }
	void setTrack(const Track& track, const std::string& path = {});
	[[nodiscard]] std::optional<TrackEditorRequest> takeRequest();

  private:
	Track _track{800, 500, 20};
	std::size_t _selected = 0;
	Vec2 _camera{};
	float _zoom = 1.0f;
	bool _fit = true, _snap = true, _linked = false, _dirty = false;
	bool _placeStart = false;
	int _drag = 0;
	Vec2 _dragMouse{}, _dragStart{};
	char _name[160] = "nouveau_circuit";
	std::vector<std::filesystem::path> _files;
	std::vector<Track> _undo, _redo;
	std::vector<std::string> _issues;
	std::string _message;
	std::optional<TrackEditorRequest> _request;
	void checkpoint();
	bool apply(std::vector<TrackSegment> segments, float width, float start, bool remember = true);
	void refreshFiles();
	void load(const std::filesystem::path& path);
	void save();
	void add(bool rounded);
	void properties();
	void canvas();
};
