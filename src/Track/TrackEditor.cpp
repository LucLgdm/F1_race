#include "Track/TrackEditor.hpp"
#include "Track/TrackIO.hpp"
#include "Track/ChampionIO.hpp"
#include "ImGui/imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <numbers>

namespace {
constexpr float pi = std::numbers::pi_v<float>;
float distance(Vec2 a, Vec2 b) {
	return std::hypot(a.x - b.x, a.y - b.y);
}
Vec2 end(const TrackSegment& s) {
	return Track::segmentPoint(s, Track::segmentLength(s));
}
float endHeading(const TrackSegment& s) {
	return Track::segmentHeading(s, Track::segmentLength(s));
}
float squaredDistance(Vec2 p, Vec2 a, Vec2 b) {
	const float x = b.x - a.x, y = b.y - a.y, l = x * x + y * y;
	const float t = l > 0 ? std::clamp(((p.x - a.x) * x + (p.y - a.y) * y) / l, 0.0f, 1.0f) : 0;
	return std::pow(p.x - a.x - t * x, 2) + std::pow(p.y - a.y - t * y, 2);
}
} // namespace
TrackEditor::TrackEditor() {
	_issues = _track.issues();
	refreshFiles();
}
void TrackEditor::setTrack(const Track& track, const std::string& path) {
	_track = track;
	_selected = 0;
	_fit = true;
	_undo.clear();
	_redo.clear();
	_dirty = false;
	_drag = 0;
	_placeStart = false;
	if (!path.empty())
		std::snprintf(_name, sizeof(_name), "%s", std::filesystem::path(path).stem().string().c_str());
	_issues = _track.issues();
	refreshFiles();
}
std::optional<TrackEditorRequest> TrackEditor::takeRequest() {
	return std::exchange(_request, std::nullopt);
}
void TrackEditor::checkpoint() {
	_undo.push_back(_track);
	if (_undo.size() > 64)
		_undo.erase(_undo.begin());
	_redo.clear();
}
bool TrackEditor::apply(std::vector<TrackSegment> segments, float width, float start, bool remember) {
	try {
		Track candidate{std::move(segments), width, start};
		if (remember)
			checkpoint();
		_track = std::move(candidate);
		_issues = _track.issues();
		_dirty = true;
		_selected = std::min(_selected, _track.segments().empty() ? 0 : _track.segments().size() - 1);
		return true;
	} catch (const std::exception& e) {
		_message = e.what();
		return false;
	}
}
void TrackEditor::refreshFiles() {
	_files.clear();
	std::error_code ec;
	for (std::filesystem::directory_iterator it{"circuits", ec}, last; !ec && it != last; it.increment(ec)) {
		if (!it->is_regular_file(ec))
			continue;
		const auto ext = it->path().extension();
		if (ext == ".track" || ext == ".f1")
			_files.push_back(it->path());
	}
	std::sort(_files.begin(), _files.end());
}
void TrackEditor::load(const std::filesystem::path& path) {
	try {
		Track candidate = [&]() {
			if (path.extension() == ".f1") {
				std::ifstream input{path};
				if (!input)
					throw IOError("Cannot open champion");
				return ChampionIO::read(input).track;
			}
			return TrackIO::load(path);
		}();
		checkpoint();
		_track = std::move(candidate);
		_selected = 0;
		_fit = true;
		_dirty = false;
		std::snprintf(_name, sizeof(_name), "%s", path.stem().string().c_str());
		_issues = _track.issues();
		_message = "Circuit charge : " + path.filename().string();
	} catch (const std::exception& e) {
		_message = e.what();
	}
}
void TrackEditor::save() {
	try {
		const std::string name = _name;
		if (name.empty() || name == "." || name == ".." ||
			name.find_first_of("/\\:*?\"<>|") != std::string::npos)
			throw IOError("Choisis un nom de fichier simple, sans chemin ni extension.");
		const auto path = std::filesystem::path("circuits") / (name + ".track");
		TrackIO::save(path, _track);
		_dirty = false;
		refreshFiles();
		_message = "Enregistre : " + path.string();
	} catch (const std::exception& e) {
		_message = e.what();
	}
}
void TrackEditor::add(bool rounded) {
	if (_track.segments().size() >= 256) {
		_message = "Maximum : 256 segments.";
		return;
	}
	auto pieces = _track.segments();
	TrackSegment s;
	s.rounded = rounded;
	s.radius = std::max(30.0f, _track.roadWidth());
	if (!pieces.empty()) {
		const auto& prev = pieces[_selected];
		s.start = end(prev);
		s.heading = endHeading(prev);
	}
	const auto index = pieces.empty() ? 0 : _selected + 1;
	pieces.insert(pieces.begin() + index, s);
	if (apply(std::move(pieces), _track.roadWidth(), _track.startDistance()))
		_selected = index;
}
void TrackEditor::properties() {
	ImGui::TextUnformatted("F1_race / EDITEUR DE CIRCUIT");
	ImGui::TextDisabled("%s", _dirty ? "Modifications non enregistrees" : "Circuit enregistre / initial");
	ImGui::InputText("Nom", _name, sizeof(_name));
	if (ImGui::Button("Enregistrer .track"))
		save();
	ImGui::SameLine();
	if (ImGui::Button("Actualiser"))
		refreshFiles();
	if (ImGui::BeginCombo("Ouvrir", "Choisir un circuit...")) {
		for (const auto& p : _files)
			if (ImGui::Selectable(p.filename().string().c_str()))
				load(p);
		if (_files.empty())
			ImGui::TextDisabled("Aucun circuit dans circuits/");
		ImGui::EndCombo();
	}
	ImGui::TextWrapped("Les .f1 existants sont lisibles. Le dessin est sauvegarde en .track ; les champions "
					   "restent en .f1.");
	if (ImGui::Button("Nouveau")) {
		checkpoint();
		_track = Track{{}, 20};
		_selected = 0;
		_issues = _track.issues();
		_dirty = true;
		std::snprintf(_name, sizeof(_name), "nouveau_circuit");
	}
	ImGui::SameLine();
	if (ImGui::Button("Rectangle")) {
		checkpoint();
		_track = Track{800, 500, 20};
		_selected = 0;
		_issues = _track.issues();
		_dirty = true;
		_fit = true;
	}
	if (ImGui::Button("Annuler") && !_undo.empty()) {
		_redo.push_back(_track);
		_track = _undo.back();
		_undo.pop_back();
		_issues = _track.issues();
		_dirty = true;
		_selected = 0;
	}
	ImGui::SameLine();
	if (ImGui::Button("Retablir") && !_redo.empty()) {
		_undo.push_back(_track);
		_track = _redo.back();
		_redo.pop_back();
		_issues = _track.issues();
		_dirty = true;
		_selected = 0;
	}
	ImGui::Separator();
	float width = _track.roadWidth();
	if (ImGui::DragFloat("Largeur (m)", &width, 0.25f, 6, 100, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
		auto pieces = _track.segments();
		for (auto& s : pieces)
			s.radius = std::max(s.radius, width / 2 + 1);
		apply(std::move(pieces), width, _track.startDistance());
	}
	if (ImGui::Button("+ Droite"))
		add(false);
	ImGui::SameLine();
	if (ImGui::Button("+ Virage"))
		add(true);
	if (ImGui::Button("Fermer par une droite") && !_track.segments().empty()) {
		auto pieces = _track.segments();
		TrackSegment s;
		s.start = end(pieces.back());
		const Vec2 target = pieces.front().start;
		s.length = distance(s.start, target);
		if (s.length >= 1) {
			s.heading = std::atan2(target.y - s.start.y, target.x - s.start.x);
			pieces.push_back(s);
			apply(std::move(pieces), _track.roadWidth(), _track.startDistance());
			_selected = _track.segments().size() - 1;
		}
	}
	ImGui::Checkbox("Accrocher les extremites", &_snap);
	ImGui::Checkbox("Deplacer aussi la suite", &_linked);
	ImGui::BeginChild("Segments", ImVec2(0, 125), true);
	for (std::size_t i = 0; i < _track.segments().size(); ++i) {
		const std::string label =
			std::to_string(i + 1) + (_track.segments()[i].rounded ? " / Virage" : " / Droite");
		if (ImGui::Selectable(label.c_str(), _selected == i))
			_selected = i;
	}
	ImGui::EndChild();
	if (!_track.segments().empty()) {
		auto pieces = _track.segments();
		auto& s = pieces[_selected];
		bool changed = false;
		ImGui::Text("Piece %d", static_cast<int>(_selected + 1));
		changed |= ImGui::Checkbox("Virage arrondi (90 deg)", &s.rounded);
		changed |= ImGui::DragFloat2("Position", &s.start.x, 0.5f, -10000, 10000, "%.2f",
									 ImGuiSliderFlags_AlwaysClamp);
		float degrees = s.heading * 180 / pi;
		if (ImGui::DragFloat("Angle (deg)", &degrees, 1, -3600, 3600, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
			s.heading = degrees * pi / 180;
			changed = true;
		}
		if (s.rounded) {
			changed |= ImGui::DragFloat("Rayon (m)", &s.radius, 0.25f, width / 2 + 1, 1000, "%.2f",
										ImGuiSliderFlags_AlwaysClamp);
			bool left = s.turn == 1;
			if (ImGui::Checkbox("Tourne a gauche", &left)) {
				s.turn = left ? 1 : -1;
				changed = true;
			}
		} else
			changed |= ImGui::DragFloat("Longueur (m)", &s.length, 0.5f, 1, 3000, "%.2f",
										ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::Button("Raccorder au precedent") && pieces.size() > 1) {
			const auto& prev = pieces[(_selected + pieces.size() - 1) % pieces.size()];
			s.start = end(prev);
			s.heading = endHeading(prev);
			changed = true;
		}
		if (changed) {
			if (_linked)
				for (std::size_t i = _selected + 1; i < pieces.size(); ++i) {
					const float turn =
						std::remainder(pieces[i].heading - endHeading(_track.segments()[i - 1]), 2 * pi);
					pieces[i].start = end(pieces[i - 1]);
					pieces[i].heading = endHeading(pieces[i - 1]) + turn;
				}
			apply(std::move(pieces), width, _track.startDistance());
		}
		if (ImGui::Button("Dupliquer") && _track.segments().size() < 256) {
			auto copy = _track.segments();
			auto s = copy[_selected];
			s.start = end(s);
			s.heading = endHeading(s);
			copy.insert(copy.begin() + _selected + 1, s);
			if (apply(std::move(copy), width, _track.startDistance()))
				++_selected;
		}
		ImGui::SameLine();
		if (ImGui::Button("Supprimer")) {
			auto copy = _track.segments();
			copy.erase(copy.begin() + _selected);
			apply(std::move(copy), width, _track.startDistance());
		}
		if (ImGui::Button("Monter") && _selected > 0) {
			auto copy = _track.segments();
			std::swap(copy[_selected], copy[_selected - 1]);
			--_selected;
			apply(std::move(copy), width, _track.startDistance());
		}
		ImGui::SameLine();
		if (ImGui::Button("Descendre") && _selected + 1 < _track.segments().size()) {
			auto copy = _track.segments();
			std::swap(copy[_selected], copy[_selected + 1]);
			++_selected;
			apply(std::move(copy), width, _track.startDistance());
		}
	}
	float start = _track.startDistance();
	if (_track.length() > 0 && ImGui::SliderFloat("Depart (m)", &start, 0, _track.length(), "%.1f"))
		apply(_track.segments(), width, start);
	if (ImGui::Button(_placeStart ? "Clique sur la piste..." : "Placer le depart"))
		_placeStart = !_placeStart;
	ImGui::SameLine();
	if (ImGui::Button("Cadrer"))
		_fit = true;
	ImGui::Separator();
	ImGui::Text("%.1f m / %d pieces", _track.length(), static_cast<int>(_track.segments().size()));
	for (const auto& issue : _issues)
		ImGui::TextWrapped("%s", issue.c_str());
	if (_issues.empty())
		ImGui::TextColored(ImVec4(0.5f, 1, 0.55f, 1), "Boucle jouable");
	ImGui::BeginDisabled(!_issues.empty());
	if (ImGui::Button("Conduire"))
		_request = TrackEditorRequest{_track, std::string("circuits/") + _name + ".track", true};
	ImGui::SameLine();
	if (ImGui::Button("Entrainer les voitures"))
		_request = TrackEditorRequest{_track, std::string("circuits/") + _name + ".track", false};
	ImGui::EndDisabled();
	ImGui::TextWrapped("%s", _message.c_str());
}
void TrackEditor::canvas() {
	const ImVec2 top = ImGui::GetCursorScreenPos(), size = ImGui::GetContentRegionAvail();
	if (size.x < 10 || size.y < 10)
		return;
	ImGui::InvisibleButton("Plan", size,
						   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle |
							   ImGuiButtonFlags_MouseButtonRight);
	const bool hovered = ImGui::IsItemHovered();
	auto& io = ImGui::GetIO();
	const ImVec2 center{top.x + size.x / 2, top.y + size.y / 2};
	if (_fit) {
		Vec2 lo{0, 0}, hi{0, 0};
		bool first = true;
		for (const auto& e : _track.boundaries())
			for (Vec2 p : {e.a, e.b}) {
				if (first) {
					lo = hi = p;
					first = false;
				}
				lo.x = std::min(lo.x, p.x);
				lo.y = std::min(lo.y, p.y);
				hi.x = std::max(hi.x, p.x);
				hi.y = std::max(hi.y, p.y);
			}
		_camera = {(lo.x + hi.x) / 2, (lo.y + hi.y) / 2};
		_zoom = std::clamp(std::min(size.x / (hi.x - lo.x + 50), size.y / (hi.y - lo.y + 50)), 0.05f, 40.0f);
		_fit = false;
	}
	const auto world = [&](ImVec2 p) -> Vec2 {
		return {_camera.x + (p.x - center.x) / _zoom, _camera.y - (p.y - center.y) / _zoom};
	};
	if (hovered && io.MouseWheel != 0) {
		const Vec2 before = world(io.MousePos);
		_zoom = std::clamp(_zoom * std::pow(1.15f, io.MouseWheel), 0.05f, 40.0f);
		const Vec2 after = world(io.MousePos);
		_camera.x += before.x - after.x;
		_camera.y += before.y - after.y;
	}
	if (hovered &&
		(ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
		_camera.x -= io.MouseDelta.x / _zoom;
		_camera.y += io.MouseDelta.y / _zoom;
	}
	const auto screen = [&](Vec2 p) -> ImVec2 {
		return {center.x + (p.x - _camera.x) * _zoom, center.y - (p.y - _camera.y) * _zoom};
	};
	Vec2 mouse = world(io.MousePos);
	if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		if (_placeStart && !_track.segments().empty()) {
			apply(_track.segments(), _track.roadWidth(), _track.progressAt(mouse) + _track.startDistance());
			_placeStart = false;
		} else {
			_drag = 0;
			if (!_track.segments().empty()) {
				const auto& s = _track.segments()[_selected];
				const Vec2 e = end(s);
				const Vec2 rotation{s.start.x - std::sin(s.heading) * (_track.roadWidth() / 2 + 18 / _zoom),
									s.start.y + std::cos(s.heading) * (_track.roadWidth() / 2 + 18 / _zoom)};
				if (distance(mouse, e) * _zoom < 10)
					_drag = 2;
				else if (distance(mouse, rotation) * _zoom < 10)
					_drag = 3;
			}
			if (!_drag) {
				float best = std::pow(_track.roadWidth() / 2 + 6 / _zoom, 2);
				std::optional<std::size_t> hit;
				for (std::size_t i = 0; i < _track.segments().size(); ++i) {
					const auto points = _track.polyline(i);
					for (std::size_t j = 1; j < points.size(); ++j) {
						const float d = squaredDistance(mouse, points[j - 1], points[j]);
						if (d < best) {
							best = d;
							hit = i;
						}
					}
				}
				if (hit) {
					_selected = *hit;
					_drag = 1;
				}
			}
			if (_drag) {
				checkpoint();
				_dragMouse = mouse;
				_dragStart = _track.segments()[_selected].start;
			}
		}
	}
	if (_drag && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
		io.MouseDelta.x * io.MouseDelta.x + io.MouseDelta.y * io.MouseDelta.y > 0) {
		auto pieces = _track.segments();
		const auto old = pieces;
		auto& s = pieces[_selected];
		if (_drag == 1) {
			s.start = {_dragStart.x + mouse.x - _dragMouse.x, _dragStart.y + mouse.y - _dragMouse.y};
			if (_snap && !io.KeyAlt) {
				for (std::size_t i = 0; i < pieces.size(); ++i)
					if (i != _selected && distance(s.start, end(pieces[i])) * _zoom < 14) {
						s.start = end(pieces[i]);
						s.heading = endHeading(pieces[i]);
						break;
					}
			}
		} else if (_drag == 3) {
			s.heading = std::atan2(mouse.y - s.start.y, mouse.x - s.start.x) - pi / 2;
			if (_snap && !io.KeyAlt)
				s.heading = std::round(s.heading / (pi / 12)) * (pi / 12);
		} else {
			const float dx = mouse.x - s.start.x, dy = mouse.y - s.start.y;
			const float x = dx * std::cos(s.heading) + dy * std::sin(s.heading),
						y = -dx * std::sin(s.heading) + dy * std::cos(s.heading);
			if (s.rounded)
				s.radius = std::clamp((x + s.turn * y) / 2, _track.roadWidth() / 2 + 1, 1000.0f);
			else
				s.length = std::clamp(x, 1.0f, 3000.0f);
		}
		if (_linked)
			for (std::size_t i = _selected + 1; i < pieces.size(); ++i) {
				const float turn = std::remainder(old[i].heading - endHeading(old[i - 1]), 2 * pi);
				pieces[i].start = end(pieces[i - 1]);
				pieces[i].heading = endHeading(pieces[i - 1]) + turn;
			}
		apply(std::move(pieces), _track.roadWidth(), _track.startDistance(), false);
	}
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
		_drag = 0;
	auto* draw = ImGui::GetWindowDrawList();
	draw->PushClipRect(top, ImVec2(top.x + size.x, top.y + size.y), true);
	draw->AddRectFilled(top, ImVec2(top.x + size.x, top.y + size.y), IM_COL32(24, 32, 28, 255));
	const float grid = std::pow(10.0f, std::ceil(std::log10(35 / _zoom)));
	const Vec2 upper = world(top), lower = world(ImVec2(top.x + size.x, top.y + size.y));
	for (float x = std::floor(upper.x / grid) * grid; x < lower.x; x += grid)
		draw->AddLine(screen({x, upper.y}), screen({x, lower.y}), IM_COL32(40, 50, 44, 255));
	for (float y = std::floor(lower.y / grid) * grid; y < upper.y; y += grid)
		draw->AddLine(screen({upper.x, y}), screen({lower.x, y}), IM_COL32(40, 50, 44, 255));
	for (const auto& t : _track.mesh())
		draw->AddTriangleFilled(screen(t.a), screen(t.b), screen(t.c), IM_COL32(80, 85, 83, 255));
	for (const auto& e : _track.boundaries())
		draw->AddLine(screen(e.a), screen(e.b), IM_COL32(220, 226, 212, 255), 1.5f);
	for (std::size_t i = 0; i < _track.segments().size(); ++i) {
		const auto points = _track.polyline(i);
		const auto color = i == _selected ? IM_COL32(216, 252, 110, 255) : IM_COL32(140, 157, 147, 255);
		for (std::size_t j = 1; j < points.size(); ++j)
			draw->AddLine(screen(points[j - 1]), screen(points[j]), color, i == _selected ? 2.5f : 1.0f);
		draw->AddCircleFilled(screen(points.front()), 3, color);
		const auto label = std::to_string(i + 1);
		draw->AddText(screen(points[points.size() / 2]), color, label.c_str());
	}
	if (!_track.segments().empty()) {
		const auto& s = _track.segments()[_selected];
		const ImVec2 e = screen(end(s));
		draw->AddRectFilled(ImVec2(e.x - 5, e.y - 5), ImVec2(e.x + 5, e.y + 5), IM_COL32(216, 252, 110, 255));
		const Vec2 rotation{s.start.x - std::sin(s.heading) * (_track.roadWidth() / 2 + 18 / _zoom),
							s.start.y + std::cos(s.heading) * (_track.roadWidth() / 2 + 18 / _zoom)};
		draw->AddLine(screen(s.start), screen(rotation), IM_COL32(216, 252, 110, 180));
		draw->AddCircleFilled(screen(rotation), 5, IM_COL32(216, 252, 110, 255));
		const auto p = _track.startPosition();
		const float a = _track.startHeading(), half = _track.roadWidth() / 2;
		draw->AddLine(screen({p.x - std::sin(a) * half, p.y + std::cos(a) * half}),
					  screen({p.x + std::sin(a) * half, p.y - std::cos(a) * half}),
					  IM_COL32(255, 190, 90, 255), 3);
		draw->AddText(screen(p), IM_COL32(255, 210, 120, 255), "DEPART >");
	}
	draw->AddText(
		ImVec2(top.x + 12, top.y + 12), IM_COL32(220, 230, 220, 255),
		"Glisser : deplacer | Carre : dimension | Rond : rotation | Molette : zoom | Clic droit : vue");
	char coordinates[120];
	std::snprintf(coordinates, sizeof(coordinates), "X %.1f  Y %.1f | Grille %.0f m | Alt : sans accrochage",
				  mouse.x, mouse.y, grid);
	draw->AddText(ImVec2(top.x + 12, top.y + size.y - 25), IM_COL32(180, 200, 186, 255), coordinates);
	draw->PopClipRect();
}
void TrackEditor::render() {
	const auto size = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(size);
	ImGui::Begin("Track Studio", nullptr,
				 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	ImGui::BeginChild("Proprietes", ImVec2(std::min(360.0f, size.x * 0.4f), 0), true);
	properties();
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Vue", ImVec2(0, 0), false,
					  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	canvas();
	ImGui::EndChild();
	ImGui::End();
}
