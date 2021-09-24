#pragma once 

#include <xenon.h>

namespace xe {

	struct OrbitCamera : Camera {
		float distance = 2, pitch = 0, yaw = 0;
		glm::vec3 center = glm::vec3(0.0f);
		glm::vec3 lerpTarget = glm::vec3(0.0f);
	};

	OrbitCamera createOrbitCamera(int width, int height, float near = 0.01f, float far = 1000.0f);
	void updateOrbitCameraProjection(OrbitCamera& camera, int width, int height, float near = 0.01f, float far = 1000.0f);
	void updateOrbitCamera(OrbitCamera& camera, Timestep ts);

	void cameraFocus(OrbitCamera& camera, glm::vec3 position);

	void renderOrbitCameraUI(OrbitCamera& camera);

}
