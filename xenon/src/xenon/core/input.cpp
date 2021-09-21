#include "input.h"

#include "xenon/core/application.h"

namespace xe {

	// TODO: Move into application struct (per application inputs)
	// TODO: Move functions into namespace

	namespace Input {

		static Application* s_currentApplication = nullptr;

		void initializeApplicationInput(Application* application) {
			s_currentApplication = application;

			glfwSetKeyCallback(application->window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
				Application* application = (Application*)glfwGetWindowUserPointer(window);
				if (action == GLFW_PRESS || action == GLFW_RELEASE) {
					application->inputManager.keyCache[key] = action == GLFW_PRESS;
				}
			});

			glfwSetMouseButtonCallback(application->window, [](GLFWwindow* window, int button, int action, int mods) {
				Application* application = (Application*)glfwGetWindowUserPointer(window);
				if (action == GLFW_PRESS || action == GLFW_RELEASE) {
					application->inputManager.mouseButtonCache[button] = action == GLFW_PRESS;
				}
			});

			glfwSetScrollCallback(application->window, [](GLFWwindow* window, double x, double y) {
				Application* application = (Application*)glfwGetWindowUserPointer(window);
				application->inputManager.mouseScrollDeltaX += x;
				application->inputManager.mouseScrollDeltaY += y;
				application->inputManager.newScrollInput = true;
			});
		}

		void updateInput(Application* application) {
			InputManager& inputManager = application->inputManager;
			double x, y;
			glfwGetCursorPos(application->window, &x, &y);
			inputManager.mouseDeltaX = (float)(x - inputManager.lastMouseX);
			inputManager.mouseDeltaY = (float)(y - inputManager.lastMouseY);
			inputManager.lastMouseX = x;
			inputManager.lastMouseY = y;

			if (!inputManager.newScrollInput) {
				inputManager.mouseScrollDeltaX = 0;
				inputManager.mouseScrollDeltaY = 0;
			}
			inputManager.newScrollInput = false;
		}


		int registerInputAction(const std::string& name) {
			InputManager& inputManager = s_currentApplication->inputManager;
			inputManager.namedInputs.emplace(name, inputManager.namedInputCounter);
			return inputManager.namedInputCounter++;
		}

		void addInputBinding(const InputPair& inputPair) {
			s_currentApplication->inputManager.inputs.insert(inputPair);
		}

		bool isActionPressed(InputAction action) {
			InputManager& inputManager = s_currentApplication->inputManager;
			const auto& [start, end] = inputManager.inputs.equal_range(action);
			for (auto it = start; it != end; ++it) {
				const InputBinding& binding = it->second;
				if (glfwGetKey(s_currentApplication->window, binding) == GLFW_PRESS) {
					return true;
				}
			}
			return false;
		}

		bool isActionReleased(InputAction action) {
			InputManager& inputManager = s_currentApplication->inputManager;
			const auto& [start, end] = inputManager.inputs.equal_range(action);
			for (auto it = start; it != end; ++it) {
				const InputBinding& binding = it->second;
				if (glfwGetKey(s_currentApplication->window, binding) == GLFW_RELEASE) {
					return true;
				}
			}
			return false;
		}

		bool isActionHeld(InputAction action) {
			InputManager& inputManager = s_currentApplication->inputManager;
			const auto& [start, end] = inputManager.inputs.equal_range(action);
			for (auto it = start; it != end; ++it) {
				const InputBinding& binding = it->second;
				if (inputManager.keyCache[binding]) {
					return true;
				}
			}
			return false;
		}

		bool isKeyPressed(unsigned int key) {
			return glfwGetKey(s_currentApplication->window, key) == GLFW_PRESS;
		}

		bool isKeyReleased(unsigned int key) {
			return glfwGetKey(s_currentApplication->window, key) == GLFW_RELEASE;
		}

		bool isKeyHeld(unsigned int key) {
			return s_currentApplication->inputManager.keyCache[key];
		}

		bool isMouseButtonPressed(unsigned int button) {
			return glfwGetMouseButton(s_currentApplication->window, button) == GLFW_PRESS;
		}

		bool isMouseButtonReleased(unsigned int button) {
			return glfwGetMouseButton(s_currentApplication->window, button) == GLFW_RELEASE;
		}

		bool isMouseButtonHeld(unsigned int button) {
			return s_currentApplication->inputManager.mouseButtonCache[button];
		}

		float getMouseDeltaX() {
			return s_currentApplication->inputManager.mouseDeltaX;
		}

		float getMouseDeltaY() {
			return s_currentApplication->inputManager.mouseDeltaY;
		}

		int getMouseX() {
			return (int)s_currentApplication->inputManager.lastMouseX;
		}

		int getMouseY() {
			return (int)s_currentApplication->inputManager.lastMouseY;
		}

		float getScrollDeltaX() {
			return s_currentApplication->inputManager.mouseScrollDeltaX;
		}

		float getScrollDeltaY() {
			return s_currentApplication->inputManager.mouseScrollDeltaY;
		}

	}

}
