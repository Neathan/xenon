#include "application.h"

#include "xenon/core/debug.h"
#include "xenon/core/input.h"

namespace xe {

	static int s_applicationCount = 0;

	Application* createApplication(const std::string& title, int width, int height) {
		if (s_applicationCount == 0) {
			if (!glfwInit()) {
				XE_LOG_CRITICAL("Failed to initialize GLFW");
				return nullptr;
			}
		}

		glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
		GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		glfwMakeContextCurrent(window);

		int version = gladLoadGL(glfwGetProcAddress);
		if (version == 0) {
			XE_LOG_CRITICAL("Failed to initialize OpenGL context");
			return nullptr;
		}

		// Enable debug
		installDebugCallback(window);

		// Enable default OpenGL features
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthFunc(GL_LEQUAL);

		glViewport(0, 0, width, height);

		Application* application = new Application{ window, title, width, height, true };
		glfwSetWindowUserPointer(window, application);

		// Input
		Input::initializeApplicationInput(application);

		// Install size callbacks
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
			Application* application = (Application*)glfwGetWindowUserPointer(window);
			application->width = width;
			application->height = height;
			application->viewportSizeChanged = true;
		});

		return application;
	}

	void destroyApplication(Application* application) {
		glfwDestroyWindow(application->window);
		delete application;

		--s_applicationCount;
		if (s_applicationCount == 0) {
			glfwTerminate();
		}
	}


	bool shouldApplicationClose(Application* application) {
		return glfwWindowShouldClose(application->window);
	}


	Timestep updateApplication(Application* application) {
		// Pre-input flag
		if (application->viewportSizeChanged) {
			// flag should only be set for one frame
			application->viewportSizeChanged = false;
		}
		// Input
		glfwPollEvents();
		Input::updateInput(application);

		// Rendering TODO: Move to rendering preparation
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Time
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - application->frameTime;
		application->frameTime = currentTime;
		
		return Timestep{ deltaTime, currentTime };
	}

	void swapBuffers(Application* application) {
		glfwSwapBuffers(application->window);
	}


	void maximizeApplication(Application* application) {
		glfwMaximizeWindow(application->window);
	}

}
