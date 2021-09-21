#include "orbit_camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

namespace xe {

	void updateCameraTransform(OrbitCamera& camera) {
		camera.transform = glm::mat4(1.0f);
		camera.transform = glm::rotate(camera.transform, glm::radians(camera.yaw), glm::vec3(0, 1, 0));
		camera.transform = glm::rotate(camera.transform, glm::radians(camera.pitch), glm::vec3(1, 0, 0));
		camera.transform = glm::translate(camera.transform, glm::vec3(0, 0, camera.distance));
		camera.inverseTransform = glm::inverse(camera.transform);
	}

	OrbitCamera createOrbitCamera(int width, int height, float near, float far) {
		OrbitCamera camera;
		updateOrbitCameraProjection(camera, width, height, near, far);
		updateCameraTransform(camera);
		return camera;
	}

	void updateOrbitCameraProjection(OrbitCamera& camera, int width, int height, float near, float far) {
		camera.projection = glm::perspectiveFov(glm::radians(80.0f), (float)width, (float)height, near, far);
	}

	void updateOrbitCamera(OrbitCamera& camera) {
		if (Input::isMouseButtonHeld(GLFW_MOUSE_BUTTON_RIGHT)) {
			camera.pitch -= Input::getMouseDeltaY();
			camera.yaw -= Input::getMouseDeltaX();
		}

		camera.distance -= Input::getScrollDeltaY() * 0.2f;
		updateCameraTransform(camera);
	}

	void renderOrbitCameraUI(OrbitCamera& camera) {
		if (ImGui::Begin("Camera")) {
			ImGui::DragFloat("Distance", &camera.distance, 0.1f);
			ImGui::DragFloat("Pitch", &camera.pitch, 0.1f);
			ImGui::DragFloat("Yaw", &camera.yaw, 0.1f);
		}
		ImGui::End();
	}

}
