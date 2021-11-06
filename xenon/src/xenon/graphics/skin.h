#pragma once

#include <vector>

#include <glm/mat4x4.hpp>

namespace xe {

	struct Skin {
		std::vector<glm::mat4> inverseBindMatrices;

		int skeleton; // Root node
		std::vector<int> joints;
		std::vector<int> jointNodes;

		std::string name;
	};

}
