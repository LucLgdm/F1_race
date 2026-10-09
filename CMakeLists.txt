cmake_minimum_required(VERSION 3.20)

project(F1_race LANGUAGES C CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(F1_race
	src/main.cpp
	src/glad.c
	src/Application.cpp
	src/Car.cpp
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