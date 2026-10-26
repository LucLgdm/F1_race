#include "Track/TrackIO.hpp"
#include "CameraView.hpp"
#include "Track/TrackEditor.hpp"
#include "Track/ChampionIO.hpp"
#include "Simulation.hpp"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <array>
#include <chrono>
#include <fstream>
#include <numbers>
#include <sstream>

namespace {
constexpr float pi = std::numbers::pi_v<float>;
Track roundedLoop() {
	std::vector<TrackSegment> pieces;
	TrackSegment s{{0, 0}, 0, 80};
	for (int i = 0; i < 4; ++i) {
		s.rounded = false;
		s.length = 80;
		pieces.push_back(s);
		s.start = Track::segmentPoint(s, 80);
		s.rounded = true;
		s.radius = 20;
		s.turn = -1;
		pieces.push_back(s);
		s.start = Track::segmentPoint(s, Track::segmentLength(s));
		s.heading -= pi / 2;
	}
	return Track{pieces, 12, 20};
}
struct EditorUI {
	EditorUI() {
		ImGui::CreateContext();
		auto& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.DisplaySize = ImVec2(1600, 1200);
		io.DeltaTime = 1.0f / 60;
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	}
	~EditorUI() { ImGui::DestroyContext(); }
	void frame(TrackEditor& editor) {
		ImGui::NewFrame();
		editor.render();
		ImGui::Render();
	}
	ImGuiWindow* window(const char* part) {
		for (auto* w : ImGui::GetCurrentContext()->Windows)
			if (std::string(w->Name).find(part) != std::string::npos)
				return w;
		return nullptr;
	}
	void activate(TrackEditor& editor, const char* label) {
		auto* properties = window("/Proprietes_");
		REQUIRE(properties != nullptr);
		ImGui::ActivateItemByID(properties->GetID(label));
		frame(editor);
	}
	void mouse(TrackEditor& editor, ImVec2 position, bool down) {
		auto& io = ImGui::GetIO();
		io.AddMousePosEvent(position.x, position.y);
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, down);
		frame(editor);
	}
};
} // namespace

TEST_CASE("Un rectangle oriente utilise ses limites locales pour les rayons et collisions",
		  "[editor][geometry]") {
	const Track track{{TrackSegment{{10, 20}, pi / 4, 80}}, 10};
	const Vec2 p = Track::segmentPoint(track.segments()[0], 40);
	REQUIRE(track.canDrive(p, 2.3f));
	REQUIRE(track.raycast(p, pi / 4 + pi / 2, 100) == Catch::Approx(5).margin(0.001));
	REQUIRE(track.raycast(p, pi / 4, 100) == Catch::Approx(40).margin(0.001));
	REQUIRE_FALSE(track.canDrive({p.x - 4, p.y + 4}, 2.3f));
	REQUIRE(track.locate(p).localDistance == Catch::Approx(40));
}
TEST_CASE("Un raccord entre deux pieces ne devient pas un mur", "[editor][geometry]") {
	const Track track{{TrackSegment{{0, 0}, 0, 40}, TrackSegment{{40, 0}, 0, 40}}, 10};
	REQUIRE(track.canDrive({40, 0}, 2.3f));
	REQUIRE(track.canTravel({39, 0}, {41, 0}, 2.3f));
	REQUIRE(track.raycast({30, 0}, 0, 100) == Catch::Approx(50));
}
TEST_CASE("Les virages a droite et a gauche partagent leurs bords avec les capteurs", "[editor][geometry]") {
	for (int turn : {-1, 1}) {
		TrackSegment s{{0, 0}, 0, 80, true, 30, turn};
		Track track{{s}, 10};
		const float distance = 30 * pi / 4;
		const Vec2 p = Track::segmentPoint(s, distance);
		const auto local = track.locate(p);
		REQUIRE(local.localDistance == Catch::Approx(distance).margin(0.001));
		REQUIRE(local.heading == Catch::Approx(turn * pi / 4).margin(0.001));
		REQUIRE(track.canDrive(p, 2.3f));
		REQUIRE(track.raycast(p, local.heading + pi / 2, 40) == Catch::Approx(5).margin(0.015));
		REQUIRE(track.raycast(p, local.heading - pi / 2, 40) == Catch::Approx(5).margin(0.015));
	}
}
TEST_CASE("Les nouveaux circuits conservent longueur et progression au passage du depart",
		  "[editor][geometry]") {
	const Track track = roundedLoop();
	REQUIRE(track.issues().empty());
	REQUIRE(track.length() == Catch::Approx(320 + 40 * pi));
	for (float d = 0; d < track.length(); d += 0.7f) {
		CAPTURE(d);
		const Vec2 p = track.pointAt(d);
		REQUIRE(track.canDrive(p, 2.3f));
		REQUIRE(std::abs(std::remainder(track.progressAt(p) - d, track.length())) < 0.001);
	}
	REQUIRE(track.progressAt(track.startPosition()) == Catch::Approx(0).margin(0.001));
}
TEST_CASE("La projection locale ne saute pas vers une voie voisine", "[editor][progress]") {
	const Track track{
		{TrackSegment{{0, 0}, 0, 100}, TrackSegment{{100, 0}, pi / 2, 20}, TrackSegment{{100, 20}, pi, 100}},
		10};
	REQUIRE(track.locate({50, 20}).segment == 2);
	const auto local = track.locate({50, 20}, 50, 3);
	REQUIRE(local.segment == 0);
	REQUIRE(local.coordinate == Catch::Approx(50));
}
TEST_CASE("Le volume balaye de la voiture ne peut traverser le terre plein", "[editor][collision]") {
	const Track track{120, 80, 20};
	REQUIRE(track.canDrive({10, 30}, 2.3f));
	REQUIRE(track.canDrive({10, -30}, 2.3f));
	REQUIRE_FALSE(track.canTravel({10, 30}, {10, -30}, 2.3f));
	REQUIRE_FALSE(track.canDrive({0, 0}, 0));
	REQUIRE_FALSE(track.canDrive({0, 30}, -1));
}
TEST_CASE("La voiture demarre et se replace selon la direction du circuit", "[editor][simulation]") {
	const Track track{{TrackSegment{{0, 0}, pi / 2, 100}}, 12};
	Simulation simulation{track, {0, 30}};
	REQUIRE(simulation.car().heading() == Catch::Approx(pi / 2));
	REQUIRE_FALSE(simulation.step(CarControls{1, 0, 0}, 0.5f));
	REQUIRE(simulation.car().position().y > 30);
	REQUIRE(std::abs(simulation.car().position().x) < 0.001);
	REQUIRE(simulation.location().coordinate > 30);
	simulation.reset();
	REQUIRE(simulation.car().heading() == Catch::Approx(pi / 2));
	REQUIRE_THROWS_AS(simulation.step({}, 0), carError);
}
TEST_CASE("Les circuits se sauvegardent sans reseau et sans perte de precision", "[editor][io]") {
	const Track source = roundedLoop();
	std::stringstream data;
	TrackIO::write(data, source);
	const Track loaded = TrackIO::read(data);
	std::stringstream expected, actual;
	TrackIO::write(expected, source);
	TrackIO::write(actual, loaded);
	REQUIRE(expected.str() == actual.str());
	REQUIRE(loaded.issues().empty());
	for (float d = 0; d < source.length(); d += 1.1f) {
		const auto p = source.pointAt(d);
		REQUIRE(loaded.raycast(p, 0.37f, 40) == source.raycast(p, 0.37f, 40));
	}
}
TEST_CASE("La lecture de circuits refuse les formats et dimensions invalides", "[editor][io]") {
	for (const auto* text :
		 {"F1_TRACK 99", "F1_TRACK 1 ROAD_WIDTH 12 START_DISTANCE 0 SEGMENTS -1",
		  "F1_TRACK 1 ROAD_WIDTH 12 START_DISTANCE 0 SEGMENTS 1 CURVE 0 0 0 3 -1 END_TRACK",
		  "F1_TRACK 1 ROAD_WIDTH 12 START_DISTANCE 0 SEGMENTS 1 STRAIGHT 0 0 0 80"}) {
		std::stringstream input{text};
		REQUIRE_THROWS_AS(TrackIO::read(input), IOError);
	}
}
TEST_CASE("Un champion sur un circuit arrondi retrouve son reseau et son trace", "[editor][champion]") {
	const Track track = roundedLoop();
	const Network network{std::vector<Layer>{Layer{8, 3, Activation::Tanh}}};
	std::stringstream data;
	ChampionIO::write(data, track, network, 42.5f);
	const auto saved = ChampionIO::read(data);
	REQUIRE(saved.track.length() == track.length());
	REQUIRE(saved.lapTime == 42.5f);
	const std::array<float, 8> inputs{{0.1f, 0.2f, 0.5f, 0.6f, 0.1f, 0.3f, 0, 1}};
	REQUIRE(saved.network.forward(inputs) == network.forward(inputs));
}
TEST_CASE("Le champion rectangle historique et le circuit des esses restent jouables",
		  "[editor][compatibility]") {
	std::ifstream input{std::string(F1_SOURCE_DIR) + "/circuits/rectangle.f1"};
	REQUIRE(input.good());
	const auto saved = ChampionIO::read(input);
	REQUIRE(saved.track.length() == Catch::Approx(2520));
	REQUIRE(saved.track.issues().empty());
	const auto esses = TrackIO::load(std::string(F1_SOURCE_DIR) + "/circuits/esses.track");
	REQUIRE(esses.issues().empty());
	for (float d = 0; d < esses.length(); d += 1) {
		CAPTURE(d);
		REQUIRE(esses.canDrive(esses.pointAt(d), 2.3f));
	}
}
TEST_CASE("ImGui ajoute annule retablit et sauvegarde un circuit", "[editor][ui]") {
	EditorUI ui;
	TrackEditor editor;
	const std::string name =
		"test_editor_save_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
	editor.setTrack(roundedLoop(), name + ".track");
	ui.frame(editor);
	ui.frame(editor);
	const auto count = editor.track().segments().size();
	ui.activate(editor, "+ Droite");
	REQUIRE(editor.track().segments().size() == count + 1);
	ui.activate(editor, "Annuler");
	REQUIRE(editor.track().segments().size() == count);
	ui.activate(editor, "Retablir");
	REQUIRE(editor.track().segments().size() == count + 1);
	ui.activate(editor, "Enregistrer .track");
	const auto path = std::filesystem::path("circuits") / (name + ".track");
	REQUIRE(std::filesystem::exists(path));
	REQUIRE(TrackIO::load(path).segments().size() == count + 1);
	ui.activate(editor, "Nouveau");
	REQUIRE(editor.track().segments().empty());
	ui.activate(editor, "Ouvrir");
	auto* popup = ui.window("##Combo_");
	REQUIRE(popup != nullptr);
	ImGui::ActivateItemByID(popup->GetID(path.filename().string().c_str()));
	ui.frame(editor);
	REQUIRE(editor.track().segments().size() == count + 1);
	std::filesystem::remove(path);
}
TEST_CASE("ImGui deplace un segment directement sur le plan", "[editor][ui]") {
	EditorUI ui;
	TrackEditor editor;
	editor.setTrack(Track{{TrackSegment{{-40, 0}, 0, 80}}, 10});
	ui.frame(editor);
	ui.frame(editor);
	auto* view = ui.window("/Vue_");
	REQUIRE(view != nullptr);
	const auto center = view->InnerRect.GetCenter();
	ui.mouse(editor, center, false);
	ui.mouse(editor, center, true);
	ui.mouse(editor, ImVec2(center.x + 50, center.y + 20), true);
	ui.mouse(editor, ImVec2(center.x + 50, center.y + 20), false);
	REQUIRE(editor.track().segments()[0].start.x > -40);
	REQUIRE(editor.track().segments()[0].start.y < 0);
}

TEST_CASE("ImGui redimensionne et oriente un segment avec ses poignees", "[editor][ui]") {
	EditorUI ui;
	TrackEditor editor;
	editor.setTrack(Track{{TrackSegment{{-40, 0}, 0, 80}}, 10});
	ui.frame(editor);
	ui.frame(editor);
	auto* view = ui.window("/Vue_");
	REQUIRE(view != nullptr);
	const ImVec2 top = view->DC.CursorStartPos;
	const ImVec2 size{view->WorkRect.Max.x - top.x, view->WorkRect.Max.y - top.y};
	const ImVec2 center{top.x + size.x / 2, top.y + size.y / 2};
	const float zoom = std::min(size.x / 130, size.y / 60);
	const ImVec2 endPoint{center.x + 40 * zoom, center.y};
	ui.mouse(editor, endPoint, false);
	ui.mouse(editor, endPoint, true);
	ui.mouse(editor, ImVec2(endPoint.x + 60, endPoint.y), true);
	ui.mouse(editor, ImVec2(endPoint.x + 60, endPoint.y), false);
	REQUIRE(editor.track().segments()[0].length > 80);
	ui.activate(editor, "Annuler");
	REQUIRE(editor.track().segments()[0].length == 80);
	const ImVec2 rotation{center.x - 40 * zoom, center.y - 5 * zoom - 18};
	ui.mouse(editor, rotation, false);
	ui.mouse(editor, rotation, true);
	const ImVec2 rotated{rotation.x + 5 * zoom + 18, center.y};
	ui.mouse(editor, rotated, true);
	ui.mouse(editor, rotated, false);
	REQUIRE(editor.track().segments()[0].heading == Catch::Approx(-pi / 2).margin(0.01));
}

TEST_CASE("ImGui autorise une boucle et bloque un circuit ouvert avant la conduite", "[editor][ui]") {
	EditorUI ui;
	TrackEditor editor;
	editor.setTrack(roundedLoop());
	ui.frame(editor);
	ui.frame(editor);
	ui.activate(editor, "Conduire");
	const auto request = editor.takeRequest();
	REQUIRE(request.has_value());
	REQUIRE(request->manual);
	editor.setTrack(Track{{TrackSegment{{0, 0}, 0, 80}}, 12});
	ui.frame(editor);
	ui.activate(editor, "Conduire");
	REQUIRE_FALSE(editor.takeRequest().has_value());
}

TEST_CASE("Un circuit ne peut pas repasser deux fois sur le meme ruban", "[editor][geometry]") {
	const auto loop = roundedLoop();
	auto segments = loop.segments();
	segments.insert(segments.end(), loop.segments().begin(), loop.segments().end());
	const Track repeated{segments, 12};
	REQUIRE_FALSE(repeated.issues().empty());
}

TEST_CASE("Les raccords referment les petits ecarts de precision des fichiers", "[editor][geometry]") {
	const Track track{{TrackSegment{{0, 0}, 0, 40}, TrackSegment{{40.001f, 0}, 0, 40}}, 10};
	REQUIRE(track.canDrive({40.0005f, 0}, 2.3f));
	REQUIRE(track.canTravel({39, 0}, {41, 0}, 2.3f));
	REQUIRE(track.raycast({30, 0}, 0, 100) == Catch::Approx(50.001f).margin(0.001));
}

TEST_CASE("Une seconde sauvegarde remplace un fichier puis se recharge", "[editor][io]") {
	const auto path =
		std::filesystem::temp_directory_path() /
		("f1_track_replace_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
		 ".track");
	TrackIO::save(path, Track{800, 500, 20});
	TrackIO::save(path, roundedLoop());
	REQUIRE(TrackIO::load(path).length() == roundedLoop().length());
	std::filesystem::remove(path);
}

TEST_CASE("La vue globale cadre tous les bords selon le format de la fenetre", "[camera]") {
	const auto track = roundedLoop();
	for (const auto size : {Vec2{1600, 900}, Vec2{900, 1600}, Vec2{500, 1200}}) {
		const auto view = fitTrackView(track, static_cast<int>(size.x), static_cast<int>(size.y));
		const float halfWidth = view.halfHeight * size.x / size.y;
		for (const auto& edge : track.boundaries())
			for (const Vec2 p : {edge.a, edge.b}) {
				REQUIRE(std::abs(p.x - view.position.x) < halfWidth);
				REQUIRE(std::abs(p.y - view.position.y) < view.halfHeight);
			}
	}
}

TEST_CASE("La vue globale suit le centre reel d un circuit deplace et oriente", "[camera]") {
	const Track track{{TrackSegment{{1000, -300}, pi / 4, 150}}, 12};
	const auto view = fitTrackView(track, 1600, 900);
	const auto expected = Track::segmentPoint(track.segments()[0], 75);
	REQUIRE(view.position.x == Catch::Approx(expected.x).margin(0.001));
	REQUIRE(view.position.y == Catch::Approx(expected.y).margin(0.001));
	REQUIRE(view.halfHeight > 0);
}

TEST_CASE("Le cadrage global tient compte de la place reservee au panneau", "[camera]") {
	const Track track{800, 500, 20};
	const auto wholeWindow = fitTrackView(track, 1600, 1200);
	const auto besidePanel = fitTrackView(track, 1250, 1200);
	REQUIRE(besidePanel.halfHeight > wholeWindow.halfHeight);
	const auto empty = fitTrackView(Track{{}, 12}, 1250, 1200);
	REQUIRE(empty.halfHeight == 30);
	REQUIRE_THROWS_AS(fitTrackView(track, 0, 1200), trackError);
	REQUIRE_THROWS_AS(fitTrackView(track, 1600, 0), trackError);
}
