/* ****************************************** */
/*   Application.hpp                         */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:54:07 by Luc      */
/*   Updated: 2026/10/05 15:54:07 by Luc      */
/* ****************************************** */

#pragma once

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <algorithm>
#include <limits>
#include <optional>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <utility>

#include "Exception.hpp"
#include "Renderer.hpp"
#include "ML/Agent.hpp"
#include "ML/Selection.hpp"
#include "Track/ChampionIO.hpp"
#include "ImGui/ImGuiLayer.hpp"
#include "Track/TrackEditor.hpp"
#include "Track/TrackIO.hpp"

const int HEIGHT = 1200;
const int WIDTH = 1600;

class Application {
	public:
		Application();
		~Application();

		void init();
		void run();
		void handleKey(CarControls&);

		void stepTraining(float dt);

	private:
		GLFWwindow* _window = nullptr;
		
		Renderer _renderer;
		TrackEditor _editor;
		bool _editing = true;
		std::optional<Simulation> _manual;
		std::string _activeTrackFile = "circuits/nouveau_circuit.track";
		bool _showSensors = true;
		bool _rendererReady = false;

		Track _track{800.0f, 500.0f, 20.0f};
		std::vector<Agent> _agents;
		bool _generationEvaluated = false;
		std::mt19937 _generator{std::random_device{}()};
		std::size_t _generation = 1;

		// Optional is a Network which doen't exist yet
		std::optional<Network> _bestNetwork;
		float _bestLapTime = std::numeric_limits<float>::infinity();

		ImGuiLayer _imGuiLayer;
		TrainingSettings _trainingSettings;
		bool _replayChampion = false;
		std::string _championFile = "circuits/rectangle.f1";

		void resetPopulation(const Network* network = nullptr, bool replay = false);
		void useEditorTrack(TrackEditorRequest request);
		void initGLFW();
		void initOpenGL();
		void evolvePopulation(std::size_t bestIndex);
		void loadChampion(const ChampionLoadRequest& request);
};