/* ****************************************** */
/*   Application.cpp                          */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:52:23 by Luc      */
/*   Updated: 2026/10/05 15:52:23 by Luc      */
/* ****************************************** */

#include "Application.hpp"

Application::Application() {}

Application::~Application() {
	if (_rendererReady)
		_renderer.shutdown();
	if (_window)
		glfwDestroyWindow(_window);
	glfwTerminate();
}

void Application::init() {
	initGLFW();					// Create the window
	initOpenGL();				// Configure OpenGl global

	_renderer.initialize();		// Initialize the renderer
	_rendererReady = true;

	// Initialize the agents, the car are positionned like a real F1 departure
	constexpr std::size_t populationSize = 20;
	constexpr float longitudinalOffset = 3.0f;
	constexpr float lateralOffset = 3.0f;
	const Vec2 center = _track.startPosition();

	_agents.reserve(populationSize);
	for (std::size_t i = 0; i < populationSize; ++i) {
		const float side = (i % 2 == 0) ? 1.0f : -1.0f;
		constexpr float distanceBeforeLine = 6.0f;

		const Vec2 startPosition{
			center.x - distanceBeforeLine - static_cast<float>(i) * longitudinalOffset,
			center.y + side * lateralOffset};
		_agents.emplace_back(_track, startPosition);
	}

	_imGuiLayer.init(_window, WIDTH, HEIGHT);
}

void Application::run() {
	// The car update is every 1/120 s
	constexpr double fixedDt = 1.0 / 120.0;
	double lastTime = glfwGetTime();
	double accumulator = 0.0;
	
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		_imGuiLayer.beginFrame();
		_imGuiLayer.render();

		const double time = glfwGetTime();
		const double frameDt = time - lastTime;
		lastTime = time;

		// Limite le rattrapage apres une pause ou un blocage.
        accumulator += std::clamp(frameDt, 0.0, 0.25);

		CarControls controls{};
		handleKey(controls);

		while (accumulator >= fixedDt) {
			for (std::size_t i = 0; i < _agents.size(); ++i) {
				Agent& agent = _agents[i];
				if (!agent.isActive())
					continue;
				
				agent.step(static_cast<float>(fixedDt));

				if (agent.trackFinished()) {
					std::cout << "Agent " << i
								<< " | Tour " << agent.lapCount()
								<< " | Temps " << agent.lastLapTime()
								<< " s" << std::endl;
				}
			}

			if (!_generationEvaluated && !_agents.empty()) {
				const bool allFinished = std::all_of(_agents.begin(), _agents.end(),
												[](const Agent& agent){return !agent.isActive();});
				
				if (allFinished) {
					std::vector<AgentScore> scores;
					scores.reserve(_agents.size());

					for (const Agent& agent : _agents)
						scores.push_back(AgentScore{agent.lapCount() > 0, agent.lastLapTime(), agent.bestProgress()});

					const std::size_t bestIndex = selectBestIndex(scores);
					const Agent& best = _agents[bestIndex];
					const AgentScore& result = scores[bestIndex];

					if (result.completedLap && result.lapTime < _bestLapTime) {
						_bestLapTime = result.lapTime;
						_bestNetwork = best.network();
						std::filesystem::create_directories("circuits");
						std::ofstream output{"circuits/rectangle.f1", std::ios::binary | std::ios::trunc};
						if (!output)
							throw IOError("Cannot open champion file for writing");
						ChampionIO::write(output, _track, *_bestNetwork, _bestLapTime);
						std::cout << "Nouveau record sauvegarde: " << _bestLapTime << " s" << std::endl;
					}

					std::cout << "Generation " << _generation
								<< " | Meilleur agent : " << bestIndex;
					if (result.completedLap) {
						std::cout << " | Tour termine en " << result.lapTime << " s";
					}else{
						std :: cout << " | Progression : " << result.progress << " m";
					}
					if (_bestNetwork.has_value())
						std::cout << " | Record global : " << _bestLapTime << " s";
					std::cout << std::endl;

					_generationEvaluated = true; 
					evolvePopulation(bestIndex);
				}
			}
			accumulator -= fixedDt;
		}

		// Draw the car here
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(_window, &width, &height);
		if (width > 0 && height > 0) {
			glClearColor(0.3f, 0.3f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			// The camera follows only the first car
			const auto leader = std::max_element(_agents.begin(), _agents.end(),
								[](const Agent& left, const Agent& right) {
									return left.progress() < right.progress();
								});
			if (leader != _agents.end()) {
				_renderer.beginFrame(leader->car().position(), width, height);
				_renderer.drawTrack(_track);
				for (const Agent& agent : _agents)
					_renderer.drawCar(agent.car());
			}
		}	

		_imGuiLayer.endFrame();
		glfwSwapBuffers(_window);
	}
}

void Application::evolvePopulation(std::size_t bestIndex) {
	// independent copies before the modification
	const Network parent = _bestNetwork.has_value() ? *_bestNetwork : _agents.at(bestIndex).network();

	std::size_t offspringIndex = 0;

	for (std::size_t i = 0; i < _agents.size(); ++i) {
		Agent& agent = _agents[i];
		if (i == bestIndex){
			agent.setNetwork(parent);
		}else{
			++offspringIndex;
			if (offspringIndex <= 6) {
				agent.setNetwork(parent);
				agent.mutateNetwork(0.05f, 0.03f, _generator);
			}else if (offspringIndex <= 12) {
				agent.setNetwork(parent);
				agent.mutateNetwork(0.1f, 0.1f, _generator);
			}else if (offspringIndex <= 17) {
				agent.setNetwork(parent);
				agent.mutateNetwork(0.2f, 0.3f, _generator);
			}else{
				if (_bestNetwork.has_value()) {
					agent.setNetwork(parent);
					agent.mutateNetwork(0.2f, 0.3f, _generator);
				}else{
					agent.randomizeNetwork();
				}
			}
		}
		agent.reset();
	}
	++_generation;
	_generationEvaluated = false;
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