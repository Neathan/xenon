#include "orbit_camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <imgui.h>

namespace xe {

	void updateCameraTransform(OrbitCamera& camera) {
		camera.offsetTransform = glm::mat4(1.0f);
		camera.offsetTransform = glm::translate(camera.offsetTransform, camera.center);
		camera.offsetTransform = glm::rotate(camera.offsetTransform, glm::radians(camera.yaw), glm::vec3(0, 1, 0));
		camera.offsetTransform = glm::rotate(camera.offsetTransform, glm::radians(camera.pitch), glm::vec3(1, 0, 0));
		camera.offsetTransform = glm::translate(camera.offsetTransform, glm::vec3(0, 0, camera.distance));
	}

	OrbitCamera createOrbitCamera(int width, int height, float near, float far) {
		OrbitCamera camera;
		updateOrbitCameraProjection(camera, width, height, near, far);
		updateCameraTransform(camera);
		return camera;
	}

	void updateOrbitCameraProjection(OrbitCamera& camera, int width, int height, float near, float far) {
		camera.projection = glm::perspectiveFov(glm::radians(60.0f), (float)width, (float)height, near, far);
		camera.near = near;
		camera.far = far;
	}

	void updateOrbitCamera(OrbitCamera& camera, Timestep ts) {
		if (Input::isMouseButtonHeld(GLFW_MOUSE_BUTTON_MIDDLE) && Input::isKeyHeld(GLFW_KEY_LEFT_SHIFT)) {
			glm::quat rotation = glm::quat(glm::radians(glm::vec3(camera.pitch, camera.yaw, 0)));
			camera.center += rotation * glm::vec3(-Input::getMouseDeltaX(), 0, 0) * camera.distance / 1000.0f;
			camera.center += rotation * glm::vec3(0, Input::getMouseDeltaY(), 0) * camera.distance / 1000.0f;
			camera.lerpTarget = camera.center;
		}
		else if (Input::isMouseButtonHeld(GLFW_MOUSE_BUTTON_MIDDLE)) {
			camera.pitch -= Input::getMouseDeltaY() * 0.5f;
			camera.yaw -= Input::getMouseDeltaX() * 0.5f;
		}

		camera.distance -= Input::getScrollDeltaY() * 0.1f * camera.distance;
		camera.distance = glm::clamp(camera.distance, 0.1f, 100.0f);

		camera.center += (camera.lerpTarget - camera.center) * ts.deltaTime * glm::max(1.0f / glm::max(glm::distance(camera.center, camera.lerpTarget), 100.0f), 10.0f);

		updateCameraTransform(camera);
	}

	void cameraFocus(OrbitCamera& camera, glm::vec3 position) {
		camera.lerpTarget = position;
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
