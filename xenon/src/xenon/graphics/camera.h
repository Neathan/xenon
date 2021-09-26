#pragma once

#include <glm/glm.hpp>

namespace xe {

	struct Camera {
		glm::mat4 transform = glm::mat4(1.0f);
		glm::mat4 inverseTransform = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
		float near = 0, far = 1;
	};

}
