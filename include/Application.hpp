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

#include "Exception.hpp"
#include "Car.hpp"
#include "Track.hpp"
#include "Renderer.hpp"
#include "CarSensors.hpp"

#include "ML/Network.hpp"
#include "ML/Loss.hpp"

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
		GLFWwindow* _window;
		Renderer _renderer;
		Track _track{120.0f, 80.0f, 10.0f};
		Car _car{Vec2{0.0f, 0.0f}};
		
		void initGLFW();
		void initOpenGL();
};