#pragma once 

#include <xenon.h>

namespace xe {

	struct OrbitCamera : Camera {
		float distance = 2, pitch = 0, yaw = 0;
	};

	OrbitCamera createOrbitCamera(int width, int height, float near = 0.01f, float far = 1000.0f);
	void updateOrbitCameraProjection(OrbitCamera& camera, int width, int height, float near = 0.01f, float far = 1000.0f);
	void updateOrbitCamera(OrbitCamera& camera);


	void renderOrbitCameraUI(OrbitCamera& camera);

}
