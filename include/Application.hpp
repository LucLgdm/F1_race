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

#include "Exception.hpp"
#include "Renderer.hpp"
#include "ML/Agent.hpp"

const int HEIGHT = 600;
const int WIDTH = 800;

class Application {
	public:
		Application();
		~Application();

		void init();
		void run();
		void handleKey(CarControls&);

	private:
		GLFWwindow* _window = nullptr;
		
		Renderer _renderer;
		bool _rendererReady = false;

		Track _track{800.0f, 500.0f, 20.0f};
		std::vector<Agent> _agents;

		void initGLFW();
		void initOpenGL();
};