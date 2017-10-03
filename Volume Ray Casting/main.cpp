#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <string>
#include <fstream>
#include <iostream>

GLuint CreateProgram(const char* vert, const char* frag);
void resize_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void update_fps();
void keyboard_input();
void SetUniform(const char* name, int x);
void SetUniform(const char* name, float x);
void SetUniform(const char* name, glm::vec3 v);
void SetUniform(const char* name, glm::vec4 v);

GLFWwindow* window;
GLuint program;

float cameraSpeed = .5f;

// Uniforms
float near = .001f;
float far = 10000.0f;
int width = 800;
int height = 600;
float aspectRatio = (float)width / (float)height;
float epsilon = 0.001f;
int maxSteps = 64;
//glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);
//glm::vec3 camRight = glm::vec3(1.0f, 0.0f, 0.0f);
//glm::vec3 camFront = glm::cross(camRight, camUp);
glm::vec3 eye = glm::vec3(0.0f, 0.0f, 2.0f);
float focalLength = 1.67f;			// Distance between eye and image

// FPS
float fpsStart = 0.0f;
int frames = 0;
bool vsync = true;
float delta;

// for calculating front, right, up
float pitch = 0.0f, yaw = 0.0f;		// no reason for roll

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	window = glfwCreateWindow(width, height, "Volume Ray Casting", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// Callbacks here
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetKeyCallback(window, key_callback);
	
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSwapInterval(1);	// VSync

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) return -1;
	
	glViewport(0, 0, width, height);

	program = CreateProgram("raymarch.vert", "test.frag");
	glUseProgram(program);

	GLfloat vertices[] = {
		-1.0f, -1.0f,
		-1.0f,  1.0f,
		1.0f, -1.0f,
		1.0f,  1.0f
	};

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	GLuint positionAttrib = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(positionAttrib);
	glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Set uniforms
	SetUniform("near", near);
	SetUniform("far", far);
	SetUniform("width", width);
	SetUniform("height", height);
	SetUniform("aspectRatio", aspectRatio);
	SetUniform("epsilon", epsilon);
	SetUniform("maxSteps", maxSteps);

	// FPS
	fpsStart = glfwGetTime();
	
	float startTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		delta = glfwGetTime() - startTime;
		startTime = glfwGetTime();

		// TODO: Input
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
			glfwSetWindowShouldClose(window, true);
		keyboard_input();

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		// Set uniforms
		// Camera
		glm::mat4 rot = glm::yawPitchRoll(yaw, pitch, 0.0f);
		glm::vec3 up = (rot * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)).xyz();
		glm::vec3 right = (rot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)).xyz();
		SetUniform("camUp", (rot * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)).xyz());
		SetUniform("camRight", (rot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)).xyz());
		SetUniform("camFront", (rot * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f)).xyz());
		SetUniform("eye", eye);
		SetUniform("focalLength", focalLength);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		update_fps();

		glfwSwapBuffers(window);
	}
}

#pragma region INPUT 
bool firstMouse = true;
float lastX, lastY;
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.005f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw -= xoffset;
	pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	float lim = glm::radians(89.0f);
	if (pitch > lim)
		pitch = lim;
	if (pitch < -lim)
		pitch = -lim;
}

void keyboard_input() {
	glm::mat4 rot = glm::yawPitchRoll(yaw, pitch, 0.0f);
	// TODO: Get a delta to move with
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		eye += cameraSpeed * delta * (rot * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f)).xyz();
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		eye -= cameraSpeed * delta * (rot * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f)).xyz();
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		eye -= (rot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)).xyz() * cameraSpeed * delta;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		eye += (rot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)).xyz() * cameraSpeed * delta;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		eye += glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed * delta;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		eye += glm::vec3(0.0f, -1.0f, 0.0f) * cameraSpeed * delta;
	if(glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
		SetUniform("maxSteps", ++maxSteps);
	if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
		SetUniform("maxSteps", --maxSteps);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cameraSpeed += yoffset * .02f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_V && action == GLFW_PRESS) {
		vsync = !vsync;
		glfwSwapInterval(vsync);
	}
}
#pragma endregion

#pragma region SET_UNIFORM
void SetUniform(const char* name, int x) {
	glUniform1i(glGetUniformLocation(program, name), x);
}

void SetUniform(const char* name, float x) {
	glUniform1f(glGetUniformLocation(program, name), x);
}

void SetUniform(const char* name, glm::vec3 v) {
	glUniform3f(glGetUniformLocation(program, name), v.x, v.y, v.z);
}

void SetUniform(const char* name, glm::vec4 v) {
	glUniform4f(glGetUniformLocation(program, name), v.x, v.y, v.z, v.w);
}
#pragma endregion

#pragma region SHADER
void CompileShader(GLuint id, const char* filepath) {
	std::string code;
	std::ifstream file(filepath, std::ios::in);
	if (!file.is_open()) {
		std::cout << "Could not read file " << filepath << ". File does not exist.\n";
		return;
	}
	std::string line = "";
	while (!file.eof()) {
		std::getline(file, line);
		code.append(line + "\n");
	}
	file.close();

	const GLchar* src = code.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(id, 512, NULL, infoLog);
		std::cout << "Shader " << filepath << " failed to load!" << infoLog << std::endl;
	}
}

GLuint CreateProgram(const char* vert, const char* frag) {
	GLuint program = glCreateProgram();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	CompileShader(vertex, vert);
	glAttachShader(program, vertex);

	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	CompileShader(fragment, frag);
	glAttachShader(program, fragment);

	glLinkProgram(program);

	// Check
	GLint success;
	GLchar infolog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, infolog);
		std::cout << "Failed to link program\n" << infolog << std::endl;
	}
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}
#pragma endregion

#pragma region RESIZE
void resize_callback(GLFWwindow* window, int w, int h) {
	glViewport(0, 0, w, h);
	width = w;
	height = h;
	aspectRatio = (float)width / (float)height;
	SetUniform("width", width);
	SetUniform("height", height);
	SetUniform("aspectRatio", aspectRatio);
}
#pragma endregion

#pragma region FPS
void update_fps() {
	frames++;
	if (glfwGetTime() - fpsStart >= 1.0f) {
		glfwSetWindowTitle(window, ("FPS: " + std::to_string(frames)).c_str());
		frames = 0;
		fpsStart = glfwGetTime();
	}
}
#pragma endregion