/* ****************************************** */
/*   Application.cpp                         */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:52:23 by Luc      */
/*   Updated: 2026/10/05 15:52:23 by Luc      */
/* ****************************************** */

#include "Application.hpp"

Application::Application() {}

Application::~Application() {
	glfwDestroyWindow(_window);
	glfwTerminate();

	_renderer.shutdown();
}

void Application::init() {
	initGLFW();					// Create the window
	initOpenGL();				// Configure OpenGl global

	_renderer.initialize();		// Initialize the renderer
	_track = Track(120.0f, 80.0f, 10.0f); 
	_car = Car(_track.startPosition());	// Initialize the car at the start position
}

void Application::run() {
	// The car update is every 1/120 s
	constexpr double fixedDt = 1.0 / 120.0;
	double lastTime = glfwGetTime();
	double accumulator = 0.0;
	
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
		glClearColor(0.3f, 0.3f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const double time = glfwGetTime();
		const double frameDt = time - lastTime;
		lastTime = time;

		// Limite le rattrapage apres une pause ou un blocage.
        accumulator += std::clamp(frameDt, 0.0, 0.25);

		CarControls controls{};
		handleKey(controls);

		while (accumulator >= fixedDt) {
			_car.update(controls, static_cast<float>(fixedDt));
			accumulator -= fixedDt;
		}

		// Draw the car here
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(_window, &width, &height);
		if (width > 0 && height > 0) {
			_renderer.beginFrame(_car.position(), width, height);
			_renderer.drawTrack(_track);
			_renderer.drawCar(_car);
		}	

		glfwSwapBuffers(_window);
	}
}

void Application::handleKey(CarControls& controls) {
	// General
	if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(_window, GLFW_TRUE);
	// Car controls
	if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)	
		controls.throttle = 1.0f;
	if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)	
		controls.brake = 1.0f;
	const bool left = glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS;
	const bool right = glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS;

	controls.steering = static_cast<float>(right) - static_cast<float>(left);
}

/*******************************
 * Initialisation
 ******************************/

void Application::initGLFW() {
	if (!glfwInit())
		throw glfwError("GLFW initialization failed");
		
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	_window = glfwCreateWindow(WIDTH, HEIGHT, "F1 race", nullptr, nullptr);
	if (!_window)
		throw glfwError("Window creation failed");
	
	glfwMakeContextCurrent(_window);
}

void Application::initOpenGL() {
	// Charger les fonctions OpenGL avec Glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw openGlError("Failed to initialize GLAD");
	
	// Configuration OpenGL
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glDisable(GL_DEPTH_TEST);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
}