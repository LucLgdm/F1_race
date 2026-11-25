cmake_minimum_required(VERSION 3.20)

project(F1_race LANGUAGES C CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(F1_race
	src/main.cpp
	src/glad.c
	src/Application.cpp
	src/Simulation.cpp
	src/Renderer.cpp
	src/Track/TrackScenery.cpp
	src/Track/Car.cpp
	src/Track/TrackEditor.cpp
	src/Track/Track.cpp
	src/Track/Bvh2D.cpp
	src/Track/TrackIO.cpp
	src/Track/LapCounter.cpp
	src/Track/ChampionIO.cpp
	src/ML/Neuron.cpp
	src/ML/Layer.cpp
	src/ML/Network.cpp
	src/ML/testML.cpp
	src/ML/Agent.cpp
	src/ML/RaceObservation.cpp
	src/ML/RaceTraining.cpp
	src/ML/RaceSession.cpp
	src/Track/CarCollision.cpp
	src/ML/ProgressTracker.cpp
	src/ImGui/ImGuiLayer.cpp
)

target_compile_features(F1_race PRIVATE cxx_std_20)

set_target_properties(F1_race PROPERTIES
	CXX_EXTENSIONS OFF
)

target_sources(F1_race PRIVATE
	src/ImGui/imgui.cpp
	src/ImGui/imgui_draw.cpp
	src/ImGui/imgui_tables.cpp
	src/ImGui/imgui_widgets.cpp
	src/ImGui/imgui_impl_glfw.cpp
	src/ImGui/imgui_impl_opengl3.cpp
)

target_include_directories(F1_race PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ImGui"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ImGui/backends"
)

target_link_libraries(F1_race PRIVATE
	glfw
	OpenGL::GL
)

# Tests automatiques
include(CTest)

if(BUILD_TESTING)
	find_package(Catch2 3 CONFIG REQUIRED)

	add_executable(F1_tests
		tests/test_loss.cpp
		tests/test_neuron.cpp
		tests/test_track.cpp
		tests/test_aabb.cpp
		tests/test_bvh.cpp
		tests/test_track_bvh.cpp
		tests/test_progress.cpp
		tests/test_selection.cpp
		tests/test_mutation.cpp
		tests/test_lap.cpp
		tests/test_champion_io.cpp
        tests/test_race.cpp
        tests/test_race_session.cpp
		tests/test_track_editor.cpp
		tests/test_training_camera.cpp
		tests/test_track_scenery.cpp
		src/Simulation.cpp
		src/Track/Car.cpp
		src/Track/TrackEditor.cpp
		src/ImGui/imgui.cpp
		src/ImGui/imgui_draw.cpp
		src/ImGui/imgui_tables.cpp
		src/ImGui/imgui_widgets.cpp
		src/ML/Neuron.cpp
		src/ML/Layer.cpp
		src/ML/Network.cpp
		src/ML/ProgressTracker.cpp
		src/ML/Agent.cpp
		src/ML/RaceObservation.cpp
		src/ML/RaceTraining.cpp
		src/ML/RaceSession.cpp
		src/Track/CarCollision.cpp
		src/Track/Track.cpp
		src/Track/Bvh2D.cpp
		src/Track/TrackScenery.cpp
		src/Track/TrackIO.cpp
		src/Track/LapCounter.cpp
		src/Track/ChampionIO.cpp
	)

	target_include_directories(F1_tests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_SOURCE_DIR}/include/ImGui"
	)

	target_compile_definitions(F1_tests PRIVATE F1_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
	target_compile_features(F1_tests PRIVATE cxx_std_20)
	target_link_libraries(F1_tests PRIVATE Catch2::Catch2WithMain)

	include(Catch)
	# Explicit groups also work with Catch2 3.7, which silently ignores ADD_TAGS_AS_LABELS.
    # Register each test once; persistence belongs to the ML branch checks too.
    catch_discover_tests(F1_tests TEST_SPEC "[ml],[champion]" PROPERTIES LABELS ml)
    catch_discover_tests(F1_tests TEST_SPEC "[editor]~[ml]~[champion]" PROPERTIES LABELS editor)
    catch_discover_tests(F1_tests TEST_SPEC "[track]~[ml]~[champion]~[editor]" PROPERTIES LABELS track)
    catch_discover_tests(F1_tests TEST_SPEC "~[ml]~[champion]~[editor]~[track]")
	
	# Code Coverage
	option(ENABLE_COVERAGE "Enable code coverage" OFF)

	if(ENABLE_COVERAGE)
		# Adapte F1_race au nom exact de la cible du jeu.
		foreach(target IN ITEMS F1_race F1_tests)
			target_compile_options(${target} PRIVATE
				--coverage
				-O0
				-g
			)

			target_link_options(${target} PRIVATE
				--coverage
			)
		endforeach()
	endif()
endif()

# Mesure des recherches geometriques sans fenetre ni rendu.
option(BUILD_BENCHMARKS "Build geometry and training benchmarks" OFF)
if(BUILD_BENCHMARKS)
	add_executable(F1_track_benchmark
		benchmarks/track_queries.cpp
		src/Track/Track.cpp
		src/Track/Bvh2D.cpp
		src/Track/TrackIO.cpp
	)
	target_include_directories(F1_track_benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
	target_compile_features(F1_track_benchmark PRIVATE cxx_std_20)
	set_target_properties(F1_track_benchmark PROPERTIES CXX_EXTENSIONS OFF)

	add_executable(F1_training_benchmark
		benchmarks/training_generation.cpp
		src/Track/Track.cpp
		src/Track/Bvh2D.cpp
		src/Track/TrackIO.cpp
		src/Track/ChampionIO.cpp
		src/Track/Car.cpp
		src/Track/LapCounter.cpp
		src/Simulation.cpp
		src/ML/Agent.cpp
		src/ML/RaceObservation.cpp
		src/ML/RaceTraining.cpp
		src/ML/RaceSession.cpp
		src/Track/CarCollision.cpp
		src/ML/Network.cpp
		src/ML/Layer.cpp
		src/ML/Neuron.cpp
		src/ML/ProgressTracker.cpp
	)
	target_include_directories(F1_training_benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
	target_compile_features(F1_training_benchmark PRIVATE cxx_std_20)
	set_target_properties(F1_training_benchmark PROPERTIES CXX_EXTENSIONS OFF)
endif()
