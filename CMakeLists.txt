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
	src/Car.cpp
	src/Track.cpp
	src/ML/Neuron.cpp
	src/ML/Layer.cpp
	src/ML/Network.cpp
	src/ML/testML.cpp
	src/ML/Agent.cpp
	src/ML/ProgressTracker.cpp
)

target_compile_features(F1_race PRIVATE cxx_std_20)

set_target_properties(F1_race PROPERTIES
	CXX_EXTENSIONS OFF
)

target_include_directories(F1_race PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
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
		tests/test_progress.cpp
		src/ML/Neuron.cpp
		src/ML/ProgressTracker.cpp
		src/Track.cpp
	)

	target_include_directories(F1_tests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	)

	target_compile_features(F1_tests PRIVATE cxx_std_20)
	target_link_libraries(F1_tests PRIVATE Catch2::Catch2WithMain)

	include(Catch)
	catch_discover_tests(F1_tests)
endif()
