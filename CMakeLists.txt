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
	src/Track/Car.cpp
	src/Track/TrackEditor.cpp
	src/Track/Track.cpp
	src/Track/TrackIO.cpp
	src/Track/LapCounter.cpp
	src/Track/ChampionIO.cpp
	src/ML/Neuron.cpp
	src/ML/Layer.cpp
	src/ML/Network.cpp
	src/ML/testML.cpp
	src/ML/Agent.cpp
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
		tests/test_progress.cpp
		tests/test_selection.cpp
		tests/test_mutation.cpp
		tests/test_lap.cpp
		tests/test_champion_io.cpp
		tests/test_track_editor.cpp
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
		src/Track/Track.cpp
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
	catch_discover_tests(F1_tests ADD_TAGS_AS_LABELS)
	
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
