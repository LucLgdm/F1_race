/* ****************************************** */
/*   Application.cpp                          */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:52:23 by Luc      */
/*   Updated: 2026/10/05 15:52:23 by Luc      */
/* ****************************************** */

#include "Application.hpp"
#include "CameraView.hpp"
#include "TrainingCamera.hpp"
#include <cmath>
#include <sstream>

Application::Application() {}

Application::~Application() {
	_imGuiLayer.shutdown();
	if (_rendererReady)
		_renderer.shutdown();
	if (_window)
		glfwDestroyWindow(_window);
	glfwTerminate();
}

void Application::init() {
	initGLFW();	  // Create the window
	initOpenGL(); // Configure OpenGl global

	_renderer.initialize(); // Initialize the renderer
	_rendererReady = true;

	resetPopulation();

	_imGuiLayer.init(_window, WIDTH, HEIGHT);
}

void Application::run() {
	constexpr double fixedDt = 1.0 / 120.0;
	double lastTime = glfwGetTime(), accumulator = 0.0;
	bool previousFastTraining = false;
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
		_imGuiLayer.beginFrame();
		float cameraPanelRight = 0.0f;
		if (_editing) {
			_editor.render();
			if (auto request = _editor.takeRequest()) {
				try {
					useEditorTrack(std::move(*request));
				} catch (const std::exception& e) {
					_editor.setError(e.what());
				}
			}
		} else if (_manual) {
			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::Begin("Conduite");
			if (ImGui::Button("Retour a l'editeur"))
				_editing = true;
			ImGui::SameLine();
			if (ImGui::Button("Replacer (R)"))
				_manual->reset();
			ImGui::TextUnformatted("Fleches / ZQSD / WASD pour conduire");
			ImGui::Checkbox("Afficher les capteurs", &_showSensors);
			ImGui::Checkbox("Vue de tout le circuit", &_trainingSettings.trackOverview);
			ImGui::Text("Vitesse : %.1f km/h", _manual->car().speed() * 3.6f);
			const auto& location = _manual->location();
			ImGui::Text("Piece %d | local %.2f m | lateral %.2f m", static_cast<int>(location.segment + 1),
						location.localDistance, location.lateralDistance);
			ImGui::Text("Progression %.1f / %.1f m", location.coordinate, _track.length());
			ImGui::Text("Tours %d | dernier %.3f s", static_cast<int>(_manual->lapCount()),
						_manual->lastLapTime());
			cameraPanelRight = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
			ImGui::End();
		} else {
			TrainingStats stats;
			stats.generation = _generation;
			stats.populationSize = _raceTraining ? _raceTraining->size() : _agents.size();
			stats.activeAgents = static_cast<std::size_t>(
				std::count_if(_agents.begin(), _agents.end(), [](const Agent& a) { return a.isActive(); }));
            if (_raceTraining) {
                stats.activeAgents=_raceTraining->activeCount();
                stats.raceSuccesses=_lastRaceScore.successes;
                stats.carContacts=_lastRaceScore.carContacts;
                stats.wallContacts=_lastRaceScore.wallContacts;
                stats.raceScenario=std::min(_raceTraining->scenarioIndex(0),_raceTraining->scenarioCount()-1);
            }
            if (_raceSession) {
                stats.populationSize=_raceSession->size(); stats.activeAgents=_raceSession->activeCount();
                stats.raceElapsed=_raceSession->elapsed(); stats.raceFinished=_raceSession->finished();
                stats.raceLapTarget=_raceSession->targetLaps(); stats.carContacts=_raceSession->contacts();
                for (const auto i : _raceSession->standings())
                    stats.standings.push_back({i,_raceSession->lapCount(i),_raceSession->status(i),_raceSession->finishTime(i)});
            }
            stats.hasChampion = _bestNetwork.has_value();
			stats.bestLapTime = stats.hasChampion ? _bestLapTime : 0;
            _imGuiLayer.render(_trainingSettings, stats);
            if (_trainingSettings.mode!=_drivingMode || _trainingSettings.fullRace!=static_cast<bool>(_raceSession)) {
                try {
                    std::optional<Network> seed=_bestNetwork;
                    if (!seed && _raceTraining) seed=_raceTraining->network(0);
                    if (_raceSession) seed=_raceSession->policy(0);
                    configureTraining(_trainingSettings.mode,seed ? &*seed : nullptr,_trainingSettings.fullRace);
                    _imGuiLayer.setError("");
                } catch (const std::exception& e) {
                    _trainingSettings.mode=_drivingMode;
                    _trainingSettings.fullRace=static_cast<bool>(_raceSession);
                    _trainingSettings.paused=true;
                    _imGuiLayer.setError(e.what());
                }
                accumulator=0.0;
            }
            if (std::exchange(_trainingSettings.restartRace,false) && _raceSession) {
                _trainingSettings.paused=true;
                try {
                    auto next=std::make_unique<RaceSession>(_track,_raceSession->policy(0),raceSettings());
                    _raceSession=std::move(next); _followedAgent.reset();
                    _trainingSettings.paused=true; _trainingSettings.accelerated=false; accumulator=0;
                    _imGuiLayer.setError("");
                } catch (const std::exception& e) { _imGuiLayer.setError(e.what()); }
            }
            cameraPanelRight = _imGuiLayer.panelRight();
			if (_imGuiLayer.takeEditorRequest()) {
				_editor.setTrack(_track, _activeTrackFile);
				_editing = true;
			}
			if (const auto request = _imGuiLayer.takeLoadRequest()) {
				try {
					loadChampion(*request);
					_imGuiLayer.setError("");
				} catch (const std::exception& e) {
					_trainingSettings.paused = true;
					_imGuiLayer.setError(e.what());
				}
				accumulator = 0;
			}
		}

		const bool acceleratedTraining = !_editing && !_manual && _trainingSettings.accelerated;

		const bool fastTraining = acceleratedTraining && !_trainingSettings.paused;

		if (fastTraining != previousFastTraining) {
			// Aucune attente de rafraîchissement pendant l'entraînement.
			glfwSwapInterval(fastTraining ? 0 : 1);

			accumulator = 0.0;
			lastTime = glfwGetTime();
			previousFastTraining = fastTraining;
		}

		const double now = glfwGetTime();
		const double frameDt = std::clamp(now - lastTime, 0.0, 0.25);
		lastTime = now;
		CarControls controls{};
		handleKey(controls);
		if (_manual && !_editing && !ImGui::GetIO().WantCaptureKeyboard && ImGui::IsKeyPressed(ImGuiKey_R))
			_manual->reset();
		if (_editing || (!_manual && _trainingSettings.paused))
			accumulator = 0;
		// else if (!_manual && _trainingSettings.accelerated) {
		// 	accumulator = 0;
		// 	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{8};
		// 	try {
		// 		do {
		// 			stepTraining(static_cast<float>(fixedDt));
		// 		} while (std::chrono::steady_clock::now() < deadline);
		// 	} catch (const std::exception& e) {
		// 		_trainingSettings.paused = true;
		// 		_imGuiLayer.setError(e.what());
		// 	}
		// } 
		else if (fastTraining) {
			accumulator = 0.0;

			using Clock = std::chrono::steady_clock;
			const auto deadline =
				Clock::now() + std::chrono::milliseconds{100};

			try {
				do {
					stepTraining(static_cast<float>(fixedDt));
				} while (Clock::now() < deadline);
			} catch (const std::exception& e) {
				_trainingSettings.paused = true;
				_imGuiLayer.setError(e.what());
			}
		}else {
			accumulator += frameDt;
			try {
				while (accumulator >= fixedDt) {
					if (_manual)
						(void)_manual->step(controls, static_cast<float>(fixedDt));
					else
						stepTraining(static_cast<float>(fixedDt));
					accumulator -= fixedDt;
				}
			} catch (const std::exception& e) {
				accumulator = 0;
				_trainingSettings.paused = true;
				_imGuiLayer.setError(e.what());
			}
		}
		int width = 0, height = 0;
		glfwGetFramebufferSize(_window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.1f, 0.16f, 0.12f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (!_editing && !acceleratedTraining && width > 0 && height > 0) {
			const auto beginCameraFrame = [&](Vec2 followedPosition) {
				CameraView view{followedPosition, 30.0f};
				int viewportLeft = 0;
				if (_trainingSettings.trackOverview) {
					// ImGui uses logical pixels, OpenGL uses framebuffer pixels (HiDPI).
					const float displayWidth = ImGui::GetIO().DisplaySize.x;
					const float scale = displayWidth > 0 ? width / displayWidth : 1.0f;
					viewportLeft = std::clamp(
						static_cast<int>(std::ceil(cameraPanelRight * scale)), 0, width - 1);
					view = fitTrackView(_track, width - viewportLeft, height, TrackScenery::viewPadding);
				}
				_renderer.beginFrame(view.position, width, height, view.halfHeight, viewportLeft);
			};
			if (_manual) {
				beginCameraFrame(_manual->car().position());
				_renderer.drawTrack(_track);
				_renderer.drawCar(_manual->car());
				if (_showSensors)
					_renderer.drawSensors(_manual->car(), _track);
            } else if (_trainingSettings.showSimulation && _raceSession) {
                _followedAgent=_raceSession->cameraTarget(_followedAgent);
                beginCameraFrame(_raceSession->car(*_followedAgent).position());
                _renderer.drawTrack(_track);
                for (std::size_t i=0;i<_raceSession->size();++i) {
                    const auto status=_raceSession->status(i);
                    // Finishers leave the active track; show the winner in the final view.
                    if (status==DriverStatus::Finished && (!_raceSession->finished() || i!=*_followedAgent)) continue;
                    const auto color=status==DriverStatus::Racing || status==DriverStatus::Finished
                        ? raceCarColor(i) : std::array<float,3>{.45f,.45f,.45f};
                    _renderer.drawCar(_raceSession->car(i),color);
                }
            } else if (_trainingSettings.showSimulation && _raceTraining) {
                // One isolated candidate world at a time; overlaying worlds would imply false collisions.
                std::size_t shown=0;
                for (std::size_t i=0;i<_raceTraining->size();++i)
                    if (_raceTraining->scenarioIndex(i)<_raceTraining->scenarioCount()) { shown=i; break; }
                const auto& episode=_raceTraining->episode(shown);
                beginCameraFrame(episode.car().position());
                _renderer.drawTrack(_track);
                _renderer.drawCar(episode.car());
                if (const auto opponent=episode.opponent())
                    _renderer.drawCar(Car{opponent->position,opponent->heading});
            } else if (_trainingSettings.showSimulation && !_agents.empty()) {
				_followedAgent = selectTrainingCameraTarget(_agents, _followedAgent);
				beginCameraFrame(_agents[*_followedAgent].car().position());
				_renderer.drawTrack(_track);
				for (const auto& agent : _agents)
					_renderer.drawCar(agent.car());
			}
		}
		_imGuiLayer.endFrame();
		glfwSwapBuffers(_window);
	}
}

void Application::resetPopulation(const Network* network, bool replay) {
    _raceSession.reset(); _trainingSettings.fullRace=false;
    if (_drivingMode==DrivingMode::Race) {
        auto training=std::make_unique<RaceTraining>(_track,network,replay ? 1 : 20);
        _agents.clear(); _followedAgent.reset();
        _raceTraining=std::move(training);
        return;
    }
    _raceTraining.reset();
	constexpr float distanceBeforeLine = 6.0f;
	constexpr float stagger = 6.0f;
	constexpr float carRadius = 2.3f;

	const std::size_t count = replay ? 1 : 20;

	// Jusqu'à 3 m de chaque côté, en conservant une marge au bord.
	const float lateralOffset = replay ? 0.0f : std::min(
		3.0f, _track.roadWidth() / 2.0f - carRadius - 0.25f);

	if (!replay && lateralOffset <= 0.0f)
		throw trackError("Track is too narrow for two starting columns");

	_followedAgent.reset();
	_agents.clear();
	_agents.reserve(count);

	for (std::size_t i = 0; i < count; ++i) {
		float coordinate = -distanceBeforeLine - static_cast<float>(i) * stagger;

		const Vec2 center = _track.pointAt(coordinate);
		const float heading = _track.headingAt(coordinate);

		// Indices pairs à gauche, indices impairs à droite.
		const float side = (i % 2 == 0) ? 1.0f : -1.0f;
		const float offset = side * lateralOffset;

		// La normale gauche à la piste vaut (-sin(angle), cos(angle)).
		Vec2 start{center.x - std::sin(heading) * offset, center.y + std::cos(heading) * offset};

		if (!_track.canDrive(start, carRadius)) {
			if (replay) {
				start = _track.startPosition();
				coordinate = 0.0f;
			} else
				throw trackError("Starting grid extends outside the track");
		}

		_agents.emplace_back(_track, start, coordinate);

		if (network)
			_agents.back().setNetwork(*network);
	}
	_generationEvaluated = false;
}

void Application::useEditorTrack(TrackEditorRequest request) {
	const auto name = std::filesystem::path(request.path).stem().string();
	if (name.empty() || name == "." || name == ".." ||
		name.find_first_of("/\\:*?\"<>|") != std::string::npos ||
		std::filesystem::path(request.path).parent_path() != std::filesystem::path("circuits"))
		throw IOError("Choisis un nom de circuit simple, sans chemin.");
	if (!request.track.issues().empty())
		throw trackError("The track is not playable");
	_followedAgent.reset();
	_agents.clear();
	_manual.reset();
	_raceTraining.reset(); _raceSession.reset();
	_track = std::move(request.track);
	_renderer.invalidateTrack();
	_activeTrackFile = request.path;
	_bestNetwork.reset();
	_bestLapTime = std::numeric_limits<float>::infinity();
	_replayChampion = false;
	_generation = 1;
	if (request.manual)
		_manual.emplace(_track, _track.startPosition());
	else configureTraining(_trainingSettings.mode,nullptr,false);

	_trainingSettings.paused = true;
	_trainingSettings.showSimulation = true;
	_trainingSettings.accelerated = false;
	_imGuiLayer.setChampionPath(_championFile);
	_imGuiLayer.setError("");
	_editing = false;
}

void Application::evolvePopulation(std::size_t bestIndex) {
	const bool refining = _bestNetwork.has_value();
	
	// independent copies before the modification
	const Network parent = refining ? *_bestNetwork : _agents.at(bestIndex).network();

	std::size_t offspringIndex = 0;

	for (std::size_t i = 0; i < _agents.size(); ++i) {
		Agent& agent = _agents[i];
		if (i == bestIndex) {
			agent.setNetwork(parent);
		}else if (refining) {
			agent.setNetwork(parent);
			const float strength = (offspringIndex % 2 == 0) ? 0.005f : 0.015f;
			agent.mutateNetwork(0.02f, strength, _generator);
			++offspringIndex;
		}else{
			++offspringIndex;
			if (offspringIndex <= 6) {
				agent.setNetwork(parent);
				agent.mutateNetwork(0.05f, 0.03f, _generator);
			} else if (offspringIndex <= 12) {
				agent.setNetwork(parent);
				agent.mutateNetwork(0.1f, 0.1f, _generator);
			} else if (offspringIndex <= 17) {
				agent.setNetwork(parent);
				agent.mutateNetwork(0.2f, 0.3f, _generator);
			} else {
				if (_bestNetwork.has_value()) {
					agent.setNetwork(parent);
					agent.mutateNetwork(0.2f, 0.3f, _generator);
				} else {
					agent.randomizeNetwork();
				}
			}
		}
		agent.reset();
	}
	_followedAgent.reset();
	++_generation;
	_generationEvaluated = false;
}

void Application::stepTraining(float dt) {
    if (_raceSession) {
        _raceSession->step(dt);
        if (_raceSession->finished()) {
            _trainingSettings.paused=true;
            _trainingSettings.accelerated=false;
        }
        return;
    }
    if (_raceTraining) { stepRaceTraining(dt); return; }
    _commands.resize(_agents.size());
    for (std::size_t i=0;i<_agents.size();++i)
        if (_agents[i].isActive()) _commands[i]=_agents[i].decide();
	for (std::size_t i = 0; i < _agents.size(); ++i) {
		Agent& agent = _agents[i];
		if (!agent.isActive())
			continue;

		agent.advance(_commands[i],dt);

		// if (agent.trackFinished()) {
		// 	std::cout << "Agent " << i << " | Tour " << agent.lapCount() << " | Temps " << agent.lastLapTime()
		// 			  << " s" << std::endl;
		// }
	}

	if (_replayChampion)
		return;

	if (!_generationEvaluated && !_agents.empty()) {
		const bool allFinished =
			std::all_of(_agents.begin(), _agents.end(), [](const Agent& agent) { return !agent.isActive(); });

		if (allFinished) {
			std::vector<AgentScore> scores;
			scores.reserve(_agents.size());

			for (const Agent& agent : _agents)
				scores.push_back(AgentScore{agent.lapCount() > 0, agent.lastLapTime(), agent.bestProgress()});

			const std::size_t bestIndex = selectBestIndex(scores);
			const Agent& best = _agents[bestIndex];
			const AgentScore& result = scores[bestIndex];

			if (result.completedLap && result.lapTime < _bestLapTime) {
				std::ostringstream output;
				ChampionIO::write(output, _track, best.network(), result.lapTime);
				TrackIO::saveText(_championFile, output.str());
				_bestLapTime = result.lapTime;
				_bestNetwork = best.network();
				std::cout << "Nouveau record sauvegarde: " << _bestLapTime << " s" << std::endl;
			}

			std::cout << "Generation " << _generation << " | Meilleur agent : " << bestIndex;
			if (result.completedLap) {
				std::cout << " | Tour termine en " << result.lapTime << " s";
			} else {
				std ::cout << " | Progression : " << result.progress << " m";
			}
			if (_bestNetwork.has_value())
				std::cout << " | Record global : " << _bestLapTime << " s";
			std::cout << std::endl;

			_generationEvaluated = true;
			evolvePopulation(bestIndex);
		}
	}
}

/*******************************
 * Tools
 ******************************/

void Application::handleKey(CarControls& controls) {
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;
	if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !_editing) {
		_editor.setTrack(_track, _activeTrackFile);
		_editing = true;
	}
	const auto pressed = [&](int key) { return glfwGetKey(_window, key) == GLFW_PRESS; };
	if (pressed(GLFW_KEY_W) || pressed(GLFW_KEY_Z) || pressed(GLFW_KEY_UP))
		controls.throttle = 1;
	if (pressed(GLFW_KEY_S) || pressed(GLFW_KEY_DOWN))
		controls.brake = 1;
	const bool left = pressed(GLFW_KEY_A) || pressed(GLFW_KEY_Q) || pressed(GLFW_KEY_LEFT);
	const bool right = pressed(GLFW_KEY_D) || pressed(GLFW_KEY_RIGHT);
	controls.steering = static_cast<float>(right) - static_cast<float>(left);
}

void Application::loadChampion(const ChampionLoadRequest& request) {
	std::ifstream input{request.path, std::ios::binary};
	if (!input)
		throw IOError("Cannont open champion file");

    SavedChampion saved = ChampionIO::read(input);
    const bool seedRace=request.mode==ChampionLoadMode::SeedRace;
    const bool fullRace=request.mode==ChampionLoadMode::StartFullRace;
    if (fullRace && saved.mode!=DrivingMode::Race)
        throw IOError("Load a _race.f1 traffic policy to launch a full race");
    if (fullRace) (void)RaceSession{saved.track,saved.network,raceSettings()};
    if (seedRace && saved.mode!=DrivingMode::Chrono)
        throw IOError("Select a chrono champion to seed course training");
    // Validate transfer before changing the active world.
    if (!fullRace && (seedRace || saved.mode==DrivingMode::Race)) {
        (void)saved.network.expandedDrivingNetwork();
        if (saved.track.roadWidth()<8.0f) throw trackError("Overtaking training requires a track at least 8 metres wide");
        for (const auto& scenario : raceScenarios(saved.track)) (void)RaceEpisode{saved.track,scenario};
    }
    _followedAgent.reset();
    _agents.clear(); _raceTraining.reset(); _raceSession.reset(); _manual.reset();
    _track=std::move(saved.track);
    _renderer.invalidateTrack();
    auto sourcePath=std::filesystem::path(request.path);
    if (saved.mode==DrivingMode::Race) {
        auto stem=sourcePath.stem().string();
        const auto raceSuffix=stem.rfind("_race");
        if (raceSuffix!=std::string::npos) {
            const auto tail=stem.substr(raceSuffix+5);
            const bool numbered=tail.size()>1 && tail[0]=='_' &&
                std::all_of(tail.begin()+1,tail.end(),[](char c){return c>='0' && c<='9';});
            if (tail.empty() || numbered) sourcePath=sourcePath.parent_path()/stem.substr(0,raceSuffix);
        }
    }
    _activeTrackFile=sourcePath.replace_extension(".track").string();
    if (seedRace) {
        configureTraining(DrivingMode::Race,&saved.network);
    } else {
        _drivingMode=saved.mode; _trainingSettings.mode=saved.mode;
        _bestNetwork=std::move(saved.network);
        _bestLapTime=saved.lapTime;
        _championFile=request.path;
        _replayChampion=request.mode==ChampionLoadMode::Replay;
        _generation=1; _lastRaceScore={};
        if (fullRace) {
            _raceSession=std::make_unique<RaceSession>(_track,*_bestNetwork,raceSettings());
            _trainingSettings.fullRace=true; _replayChampion=false;
        } else {
            resetPopulation(&*_bestNetwork,_replayChampion);
            if (!_replayChampion && _drivingMode==DrivingMode::Chrono) evolvePopulation(0);
        }
        _imGuiLayer.setChampionPath(_championFile);
    }
    _editing=false;

	_trainingSettings.paused = true;
	_trainingSettings.showSimulation = true;
	if (_replayChampion || fullRace)
		_trainingSettings.accelerated = false;
}

/*******************************
 * Initialisation
 ******************************/

void Application::initGLFW() {
	glfwSetErrorCallback([](int code, const char* description) {
		std::cerr << "GLFW " << code << ": " << description << '\n';
	});
	if (!glfwInit())
		throw glfwError("GLFW initialization failed");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	_window = glfwCreateWindow(WIDTH, HEIGHT, "F1 race", nullptr, nullptr);
	if (!_window)
		throw glfwError("Window creation failed");

	glfwMakeContextCurrent(_window);
	glfwSwapInterval(1);
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

RaceSettings Application::raceSettings() const {
    RaceSettings settings;
    settings.cars=static_cast<std::size_t>(_trainingSettings.raceCars);
    settings.laps=static_cast<std::size_t>(_trainingSettings.raceLaps);
    return settings;
}
void Application::configureTraining(DrivingMode mode, const Network* seed, bool fullRace) {
    fullRace=fullRace && mode==DrivingMode::Race;
    const std::optional<Network> transferred=(mode==DrivingMode::Race && seed && (!fullRace || seed->topology().front()==23))
        ? std::optional<Network>{fullRace ? *seed : seed->expandedDrivingNetwork()} : std::nullopt;
    std::optional<Network> champion;
    float bestTime=std::numeric_limits<float>::infinity();
    auto path=std::filesystem::path(_activeTrackFile);
    const auto stem=path.stem().string()+(mode==DrivingMode::Race ? "_race" : "");
    path=path.parent_path()/(stem+".f1");
    int suffix=1;
    while (std::filesystem::exists(path)) {
        try {
            std::ifstream input{path};
            auto saved=ChampionIO::read(input);
            std::ostringstream a,b;
            TrackIO::write(a,_track); TrackIO::write(b,saved.track);
            if (saved.mode==mode && a.str()==b.str()) {
                champion=std::move(saved.network); bestTime=saved.lapTime; break;
            }
        } catch (const std::exception&) { /* preserve incompatible/unreadable files */ }
        path=path.parent_path()/(stem+"_"+std::to_string(++suffix)+".f1");
    }
    const Network* initial=champion ? &*champion : (transferred ? &*transferred : nullptr);
    // Construct race worlds before committing the mode; a rejected circuit keeps chrono usable.
    std::unique_ptr<RaceTraining> race;
    std::unique_ptr<RaceSession> session;
    if (fullRace) {
        if (!initial) throw IOError("Train overtaking first or load a _race.f1 traffic policy");
        session=std::make_unique<RaceSession>(_track,*initial,raceSettings());
    } else if (mode==DrivingMode::Race) race=std::make_unique<RaceTraining>(_track,initial);
    _drivingMode=mode; _trainingSettings.mode=mode;
    _bestNetwork=std::move(champion); _bestLapTime=bestTime;
    _championFile=path.string(); _replayChampion=false;
    _generation=1; _lastRaceScore={}; _followedAgent.reset();
    _agents.clear(); _raceTraining=std::move(race); _raceSession=std::move(session);
    _trainingSettings.fullRace=fullRace;
    if (mode==DrivingMode::Chrono) resetPopulation(_bestNetwork ? &*_bestNetwork : nullptr);
    _trainingSettings.paused=true;
    if (fullRace) { _trainingSettings.showSimulation=true; _trainingSettings.accelerated=false; }
    _imGuiLayer.setChampionPath(_championFile);
}
void Application::stepRaceTraining(float dt) {
    if (_raceTraining->finished()) return;
    _raceTraining->step(dt);
    if (!_raceTraining->finished()) return;
    const auto best=_raceTraining->bestIndex();
    _lastRaceScore=_raceTraining->score(best);
    if (_replayChampion) return;
    if (_lastRaceScore.successes==_raceTraining->scenarioCount() && _lastRaceScore.time<_bestLapTime) {
        std::ostringstream output;
        ChampionIO::write(output,_track,_raceTraining->network(best),_lastRaceScore.time,DrivingMode::Race);
        TrackIO::saveText(_championFile,output.str());
        _bestLapTime=_lastRaceScore.time; _bestNetwork=_raceTraining->network(best);
    }
    std::cout << "Course | Generation " << _generation << " | Reussites " << _lastRaceScore.successes
              << "/5 | Contacts voitures " << _lastRaceScore.carContacts
              << " | Contacts piste " << _lastRaceScore.wallContacts << '\n';
    _raceTraining->evolve(_generator);
    ++_generation;
}
