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

#include "Exception.hpp"
#include "Renderer.hpp"
#include "ML/Agent.hpp"
#include "ML/Selection.hpp"

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
		bool _generationEvaluated = false;
		std::mt19937 _generator{std::random_device{}()};
		std::size_t _generation = 1;

		// Optional is a Network which doen't exist yet
		std::optional<Network> _bestNetwork;
		float _bestLapTime = std::numeric_limits<float>::infinity();

		void initGLFW();
		void initOpenGL();
		void evolvePopulation(std::size_t bestIndex);
};