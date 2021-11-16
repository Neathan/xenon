#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace xe {

	void updateCameraProjection(CameraComponent& cameraComponent) {
		if (cameraComponent.projectionType == ProjectionType::Perspective) {
			cameraComponent.camera.projection = glm::perspectiveFov(
				glm::radians(cameraComponent.fov),
				(float)cameraComponent.width,
				(float)cameraComponent.height,
				cameraComponent.camera.near,
				cameraComponent.camera.far
			);
		}
		else if (cameraComponent.projectionType == ProjectionType::Orthographic) {
			cameraComponent.camera.projection = glm::orthoRH(
				0.0f,
				(float)cameraComponent.width,
				0.0f,
				(float)cameraComponent.height,
				cameraComponent.camera.near,
				cameraComponent.camera.far
			);
		}
	}

}

