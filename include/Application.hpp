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
#include "renderer.hpp"

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
		Car _car;
		Renderer _renderer;
		
		void initGLFW();
		void initOpenGL();
};