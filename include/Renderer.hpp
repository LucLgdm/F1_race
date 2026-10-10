/* ****************************************** */
/*   Renderer.hpp                         	  */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/10 17:25:21 by Luc      */
/*   Updated: 2026/10/10 17:25:21 by Luc      */
/* ****************************************** */

#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Exception.hpp"
#include "Car.hpp"

class Renderer {
	public:
		void initialize();
		void initializeShader();
		void shutdown();

		void drawCar(const Car& car, int frameBufferWidth, int frameBufferHeight);

	private:
		GLuint _carVAO = 0;
		GLuint _carVBO = 0;
		GLuint _shaderProgram = 0;
};
