#pragma once

#include <string>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "xenon/core/log.h"
#include "xenon/core/time.h"
#include "xenon/core/input.h"

namespace xe {

	struct Application {
		GLFWwindow* window;
		std::string title;
		int width, height;

		// Runtime
		bool running;
		float frameTime = 0.0;
		InputManager inputManager = InputManager();
		bool viewportSizeChanged = false;
	};

	// State management
	Application* createApplication(const std::string& title, int width, int height);
	void destroyApplication(Application* application);
	
	// State feedback
	bool shouldApplicationClose(Application* application);

	// Systems
	Timestep updateApplication(Application* application);
	void swapBuffers(Application* application);

	// Utility
	void maximizeApplication(Application* application);

}
