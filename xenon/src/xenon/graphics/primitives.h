#pragma once

#include "xenon/graphics/model.h"

namespace xe {


	enum class GeneratorDirection : unsigned char {
		UP,
		DOWN,
		LEFT,
		RIGHT,
		FRONT,
		BACK
	};

	// TODO: Investigate how a generated model behaves after model destruction
	Model* generatePlaneModel(float width, float height, GeneratorDirection facing);
	Model* generateCubeModel(glm::vec3 size);

}
