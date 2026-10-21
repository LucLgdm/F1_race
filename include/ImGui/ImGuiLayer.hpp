/* ****************************************** */
/*   ImGuiLayer.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/21 10:09:01 by Luc      */
/*   Updated: 2026/10/21 10:09:01 by Luc      */
/* ****************************************** */

#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include "ImGui/imgui.h"
#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_opengl3.h>

#include "ImGui/backends/ImGuiFileDialog.h"

#include "ImGuizmo/ImGuizmo.h"
#include "ImGui/backends/imgui_spectrum.h"

#include <iostream>

struct TrainingSettings {
	bool paused = true;
	bool showSimulation = true;
	bool accelerated = false;
	bool showSensors = false;
};

struct TrainingStats {
	std::size_t generation = 1;
	std::size_t activeAgents = 0;
	std::size_t populationSize = 0;

	bool hasChampion = false;
	float bestLapTime = 0.0f;
};

class ImGuiLayer {
	public:
		ImGuiLayer() = default;
		~ImGuiLayer();

		void init(GLFWwindow* window, int width, int height);
		void beginFrame();

		// void render(TrainingSettings& settings, const TrainingStats& stats);
		void render();
		
		void endFrame();
		void shutdown();

	private:
		bool _initialized = false;
		float _openglwidth;
		float _openglheight;
		float _viewPortWidth;
		float _viewPortHeight;

};