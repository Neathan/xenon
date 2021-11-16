#pragma once

#include <map>

#include <glm/glm.hpp>

#include "xenon/core/uuid.h"

namespace xe {

	struct Camera {
		float near = 0.1f, far = 100.0f;

		glm::mat4 projection = glm::mat4(1.0f);
		glm::mat4 offsetTransform = glm::mat4(1.0f);
		// NOTE: Offset is primarily used by the editor camera
	};

	enum class ProjectionType {
		Perspective,
		Orthographic
	};

	struct CameraComponent {
		Camera camera;
		float fov = 60.0f;
		int width = 1920, height = 1080;
		ProjectionType projectionType = ProjectionType::Perspective;
	};

	void updateCameraProjection(CameraComponent& cameraComponent);

}
