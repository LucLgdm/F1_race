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

#include <ImGui/imgui.h>
#include <ImGui/backends/imgui_impl_glfw.h>
#include <ImGui/backends/imgui_impl_opengl3.h>




#include "ImGui/backends/imgui_spectrum.h"
#include "ImGui/TrainingUI.hpp"

#include <iostream>
#include <optional>
#include <utility>

class ImGuiLayer {
	public:
		ImGuiLayer() = default;
		~ImGuiLayer();

		void init(GLFWwindow* window, int width, int height);
		void beginFrame();

		void render(TrainingSettings& settings, const TrainingStats& stats);
		
		void endFrame();
		void shutdown();

		[[nodiscard]] std::optional<ChampionLoadRequest> takeLoadRequest();

		void setError(std::string message);
		void setChampionPath(const std::string& path);
		bool takeEditorRequest() { return std::exchange(_editorRequest, false); }


	private:
		bool _initialized = false;
		bool _editorRequest = false;
		float _openglwidth;
		float _openglheight;
		float _viewPortWidth;
		float _viewPortHeight;

		char _championPath[512] = "circuits/rectangle.f1";
		std::optional<ChampionLoadRequest> _loadRequest;
		std::string _errorMessage;
};
