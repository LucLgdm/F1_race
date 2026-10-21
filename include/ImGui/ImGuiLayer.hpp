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

#include "ImGui/backends/ImGuiFileDialog.h"

#include "ImGuizmo/ImGuizmo.h"
#include "ImGui/backends/imgui_spectrum.h"
#include "ImGui/TrainingUI.hpp"

#include <iostream>

class ImGuiLayer {
	public:
		ImGuiLayer() = default;
		~ImGuiLayer();

		void init(GLFWwindow* window, int width, int height);
		void beginFrame();

		void render(TrainingSettings& settings, const TrainingStats& stats);
		
		void endFrame();
		void shutdown();


	private:
		bool _initialized = false;
		float _openglwidth;
		float _openglheight;
		float _viewPortWidth;
		float _viewPortHeight;
};
