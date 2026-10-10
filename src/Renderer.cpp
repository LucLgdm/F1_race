/* ****************************************** */
/*   Renderer.cpp                             */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/10 17:26:19 by Luc      */
/*   Updated: 2026/10/10 17:26:19 by Luc      */
/* ****************************************** */

#include "Renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


/*******************************
 * Error check
 ******************************/
static void checkShader(GLuint shader) {
	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (success != GL_TRUE)
	{
		char log[4096]{};
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		throw std::runtime_error(
			std::string("Shader compilation failed:\n") + log
		);
	}
}

/*******************************
 * Initialisation
 ******************************/

void Renderer::initialize() {
	// Coordonnées locales en mètres.
	// Longueur : 4 m sur X ; largeur : 2 m sur Y.
	// L'avant de la voiture pointe vers +X.
	constexpr float vertices[] = {
		// Premier triangle
		-2.0f, -1.0f,
		2.0f, -1.0f,
		2.0f,  1.0f,

		// Deuxième triangle
		-2.0f, -1.0f,
		2.0f,  1.0f,
		-2.0f,  1.0f
	};

	glGenVertexArrays(1, &_carVAO);
	glGenBuffers(1, &_carVBO);

	glBindVertexArray(_carVAO);

	glBindBuffer(GL_ARRAY_BUFFER, _carVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(
		0,                  // Numéro de l'attribut dans le shader
		2,                  // Deux composantes : x, y
		GL_FLOAT,
		GL_FALSE,
		2 * sizeof(float),  // Distance entre deux sommets
		nullptr             // Premier sommet au début du VBO
	);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

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
		throw std::runtime_error(
			std::string("Shader linking failed:\n") + log
		);
	}

	glDeleteShader(vShader);
	glDeleteShader(fShader);
}

/*******************************
 * Drawing
 ******************************/

void Renderer::drawCar(const Car& car, int width, int height) {
	if (width <= 0 || height <= 0)
		return;

	glViewport(0, 0, width, height);

	// Zone visible : 40 mètres de hauteur.
	// La largeur dépend du format de la fenêtre.
	const float aspect = static_cast<float>(width) / static_cast<float>(height);

	constexpr float halfHeight = 20.0f;
	const float halfWidth = halfHeight * aspect;

	const glm::mat4 projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);

	const Vec2 position = car.position();

	glm::mat4 model = glm::translate(glm::mat4{1.0f}, glm::vec3{position.x, position.y, 0.0f});

	model = glm::rotate(model, car.heading(), glm::vec3{0.0f, 0.0f, 1.0f});

	glUseProgram(_shaderProgram);

	const GLint modelLocation = glGetUniformLocation(_shaderProgram, "model");
	const GLint colorLocation = glGetUniformLocation(_shaderProgram, "objectColor");

	glUniformMatrix4fv(glGetUniformLocation(_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glBindVertexArray(_carVAO);

	// Wheels position
	const glm::vec3 wheelPositions[] = {
		glm::vec3{1.2f, 1.1f, 0.0f},	// Front Left
		glm::vec3{1.2f, -1.1f, 0.0f},	// Front Right
		glm::vec3{-1.2f, 1.1f, 0.0f},	// Rear Left
		glm::vec3{-1.2f, -1.1f, 0.0f}	// Rear Right
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


void Renderer::shutdown() {
	glDeleteBuffers(1, &_carVBO);
	glDeleteVertexArrays(1, &_carVAO);

	_carVBO = 0;
	_carVAO = 0;

	glDeleteProgram(_shaderProgram);
	_shaderProgram = 0;
}