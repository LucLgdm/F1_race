/* ****************************************** */
/*   Renderer.cpp                             */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/10 17:26:19 by Luc      */
/*   Updated: 2026/10/10 17:26:19 by Luc      */
/* ****************************************** */

#include "Renderer.hpp"
#include "Track/CarSensors.hpp"
#include <numbers>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/*******************************
 * Error check
 ******************************/
static void checkShader(GLuint shader) {
	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (success != GL_TRUE) {
		char log[4096]{};
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		throw std::runtime_error(std::string("Shader compilation failed:\n") + log);
	}
}

/*******************************
 * Initialisation
 ******************************/

void Renderer::initialize() {
	// Coordonnées locales en mètres.
	// Longueur : 4 m sur X ; largeur : 2 m sur Y.
	// L'avant de la voiture pointe vers +X.
	constexpr float vertices[] = {// Premier triangle
								  -2.0f, -1.0f, 2.0f, -1.0f, 2.0f, 1.0f,

								  // Deuxième triangle
								  -2.0f, -1.0f, 2.0f, 1.0f, -2.0f, 1.0f};

	glGenVertexArrays(1, &_carVAO);
	glGenBuffers(1, &_carVBO);

	glBindVertexArray(_carVAO);

	glBindBuffer(GL_ARRAY_BUFFER, _carVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, // Numéro de l'attribut dans le shader
						  2, // Deux composantes : x, y
						  GL_FLOAT, GL_FALSE,
						  2 * sizeof(float), // Distance entre deux sommets
						  nullptr			 // Premier sommet au début du VBO
	);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &_trackVAO);
	glGenBuffers(1, &_trackVBO);
	glBindVertexArray(_trackVAO);
	glBindBuffer(GL_ARRAY_BUFFER, _trackVBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), nullptr);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	initializeShader();
}

static std::string readFile(const char* path) {
	std::ifstream file(path);
	if (!file.is_open())
		throw std::runtime_error(std::string("Cannot open shader file: ") + path);

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void Renderer::initializeShader() {
	const char* vPath = "shaders/vertex.glsl";
	const char* fPath = "shaders/fragment.glsl";

	std::string vCode = readFile(vPath);
	std::string fCode = readFile(fPath);

	const char* vSrc = vCode.c_str();
	const char* fSrc = fCode.c_str();

	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vShader, 1, &vSrc, nullptr);
	glCompileShader(vShader);
	checkShader(vShader);

	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fShader, 1, &fSrc, nullptr);
	glCompileShader(fShader);
	checkShader(fShader);

	_shaderProgram = glCreateProgram();
	glAttachShader(_shaderProgram, vShader);
	glAttachShader(_shaderProgram, fShader);
	glLinkProgram(_shaderProgram);

	// Check for linking errors
	GLint success = GL_FALSE;
	glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);

	if (success != GL_TRUE) {
		char log[4096]{};
		glGetProgramInfoLog(_shaderProgram, sizeof(log), nullptr, log);
		throw std::runtime_error(std::string("Shader linking failed:\n") + log);
	}

	glDeleteShader(vShader);
	glDeleteShader(fShader);
}

/*******************************
 * Drawing
 ******************************/

void Renderer::beginFrame(Vec2 cameraPosition, int frameBufferWidth, int frameBufferHeight,
						  float halfHeight, int viewportLeft) {
	if (frameBufferWidth <= 0 || frameBufferHeight <= 0)
		return;

	if (!std::isfinite(halfHeight) || halfHeight <= 0.0f)
		throw openGlError("Camera half-height must be finite and positive");
	viewportLeft = std::clamp(viewportLeft, 0, frameBufferWidth - 1);
	const int viewportWidth = frameBufferWidth - viewportLeft;
	glViewport(viewportLeft, 0, viewportWidth, frameBufferHeight);
	glUseProgram(_shaderProgram);
	// halfHeight controls the orthographic zoom; position controls the view centre.
	const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(frameBufferHeight);

	const float halfWidth = halfHeight * aspect;

	const glm::mat4 projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
	const glm::mat4 view =
		glm::translate(glm::mat4{1.0f}, glm::vec3{-cameraPosition.x, -cameraPosition.y, 0.0f});

	glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "projection"), 1, GL_FALSE,
					   glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
}

void Renderer::drawCar(const Car& car) {
	const Vec2 position = car.position();

	glm::mat4 model = glm::translate(glm::mat4{1.0f}, glm::vec3{position.x, position.y, 0.0f});
	model = glm::rotate(model, car.heading(), glm::vec3{0.0f, 0.0f, 1.0f});

	glUseProgram(_shaderProgram);

	const GLint modelLocation = glGetUniformLocation(_shaderProgram, "model");
	const GLint colorLocation = glGetUniformLocation(_shaderProgram, "objectColor");

	glBindVertexArray(_carVAO);

	// Wheels position
	const glm::vec3 wheelPositions[] = {
		glm::vec3{1.2f, 1.1f, 0.0f},  // Front Left
		glm::vec3{1.2f, -1.1f, 0.0f}, // Front Right
		glm::vec3{-1.2f, 1.1f, 0.0f}, // Rear Left
		glm::vec3{-1.2f, -1.1f, 0.0f} // Rear Right
	};
	glUniform3f(colorLocation, 0.02f, 0.02f, 0.02f); // Dark color for wheels

	for (const glm::vec3& wheelPosition : wheelPositions) {
		glm::mat4 wheelModel = glm::translate(model, wheelPosition);
		// Only the front wheels rotate
		if (wheelPosition.x > 0.0f) {
			wheelModel = glm::rotate(wheelModel, car.steeringAngle(), glm::vec3{0.0f, 0.0f, 1.0f});
		}

		// Scale down the wheels to be smaller than the car body
		wheelModel = glm::scale(wheelModel, glm::vec3{0.2f, 0.2f, 1.0f});

		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(wheelModel));
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	// Draw the car body
	glUniform3f(colorLocation, 1.0f, 0.2f, 0.2f);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Renderer::drawTrack(const Track& track) {
	glUseProgram(_shaderProgram);
	const glm::mat4 identity{1.0f};
	glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(identity));
	const GLint color = glGetUniformLocation(_shaderProgram, "objectColor");
	std::vector<Vec2> vertices;
	vertices.reserve(track.mesh().size() * 3);
	for (const auto& t : track.mesh()) {
		vertices.push_back(t.a);
		vertices.push_back(t.b);
		vertices.push_back(t.c);
	}
	glBindVertexArray(_trackVAO);
	glBindBuffer(GL_ARRAY_BUFFER, _trackVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vec2), vertices.data(), GL_STREAM_DRAW);
	glUniform3f(color, 0.28f, 0.3f, 0.29f);
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
	vertices.clear();
	for (const auto& e : track.boundaries()) {
		vertices.push_back(e.a);
		vertices.push_back(e.b);
	}
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vec2), vertices.data(), GL_STREAM_DRAW);
	glUniform3f(color, 0.85f, 0.88f, 0.82f);
	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
	const auto p = track.startPosition();
	const float a = track.startHeading(), half = track.roadWidth() / 2;
	const Vec2 line[] = {{p.x - std::sin(a) * half, p.y + std::cos(a) * half},
						 {p.x + std::sin(a) * half, p.y - std::cos(a) * half}};
	glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STREAM_DRAW);
	glUniform3f(color, 1, 0.7f, 0.3f);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
}

void Renderer::drawSensors(const Car& car, const Track& track) {
	const auto distances = measureWallDistances(car, track, 40);
	std::vector<Vec2> lines;
	const Vec2 p = car.position();
	for (std::size_t i = 0; i < distances.size(); ++i) {
		const float angle = car.heading() + std::numbers::pi_v<float> / 2 - i * std::numbers::pi_v<float> / 4;
		lines.push_back(p);
		lines.push_back({p.x + std::cos(angle) * distances[i], p.y + std::sin(angle) * distances[i]});
	}
	const glm::mat4 identity{1};
	glUseProgram(_shaderProgram);
	glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(identity));
	glUniform3f(glGetUniformLocation(_shaderProgram, "objectColor"), 0.5f, 1, 0.45f);
	glBindVertexArray(_trackVAO);
	glBindBuffer(GL_ARRAY_BUFFER, _trackVBO);
	glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(Vec2), lines.data(), GL_STREAM_DRAW);
	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
	glBindVertexArray(0);
}

void Renderer::shutdown() {
	glDeleteBuffers(1, &_trackVBO);
	glDeleteVertexArrays(1, &_trackVAO);
	_trackVBO = _trackVAO = 0;
	glDeleteBuffers(1, &_carVBO);
	glDeleteVertexArrays(1, &_carVAO);

	_carVBO = 0;
	_carVAO = 0;

	glDeleteProgram(_shaderProgram);
	_shaderProgram = 0;
}