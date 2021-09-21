#pragma once

#include <array>
#include <map>
#include <string>

#include <GLFW/glfw3.h>

namespace xe {

	struct Application;

	typedef unsigned int InputAction;
	typedef int InputBinding;

	typedef std::pair<InputAction, InputBinding> InputPair;

	struct InputManager {
		std::array<bool, GLFW_KEY_LAST> keyCache;
		std::array<bool, GLFW_MOUSE_BUTTON_LAST> mouseButtonCache;

		std::map<std::string, unsigned int> namedInputs;
		unsigned int namedInputCounter = 0;

		std::multimap<InputAction, InputBinding> inputs;

		// TODO: Add check if mouse is outside window
		double lastMouseX = 0, lastMouseY = 0;
		float mouseDeltaX = 0, mouseDeltaY = 0;

		float mouseScrollDeltaX = 0, mouseScrollDeltaY = 0;
		bool newScrollInput = false;
	};

	namespace Input {

		void initializeApplicationInput(Application* application);
		void updateInput(Application* application);

		int registerInputAction(const std::string& name);
		void addInputBinding(const InputPair& inputPair);

		bool isActionPressed(InputAction action);
		bool isActionReleased(InputAction action);
		bool isActionHeld(InputAction action);

		bool isKeyPressed(unsigned int key);
		bool isKeyReleased(unsigned int key);
		bool isKeyHeld(unsigned int key);

		bool isMouseButtonPressed(unsigned int button);
		bool isMouseButtonReleased(unsigned int button);
		bool isMouseButtonHeld(unsigned int button);

		float getMouseDeltaX();
		float getMouseDeltaY();

		int getMouseX();
		int getMouseY();

		float getScrollDeltaX();
		float getScrollDeltaY();

	}

}
