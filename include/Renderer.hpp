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
#include <array>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Exception.hpp"
#include "Track/Track.hpp"
#include "Track/Car.hpp"
#include "Track/TrackScenery.hpp"

class Renderer {
	public:
		void initialize();
		void initializeShader();
		void shutdown();

		void beginFrame(Vec2 cameraPosition, int frameBufferWidth, int frameBufferHeight,
			float halfHeight = 30.0f, int viewportLeft = 0);
		void drawCar(const Car& car, std::array<float,3> color = {1.0f,0.2f,0.2f});
		void drawTrack(const Track& track);
		// Call after replacing/editing the same Track object.
		void invalidateTrack() noexcept { _sceneReady = false; }
		void drawSensors(const Car& car, const Track& track);

	private:
		GLuint _trackVAO = 0, _trackVBO = 0;
		GLuint _sceneVAO = 0, _sceneVBO = 0;
		GLsizei _sceneTriangles = 0, _sceneLines = 0;
		bool _sceneReady = false;
		const Track* _sceneTrack = nullptr;
		void cacheTrack(const Track& track);
		GLuint _carVAO = 0;
		GLuint _carVBO = 0;
		GLuint _shaderProgram = 0;
};
