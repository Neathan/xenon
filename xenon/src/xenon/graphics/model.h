#pragma once

#include <cstdint>
#include <vector>

#include <glm/mat4x4.hpp>

#include "xenon/graphics/primitive.h"
#include "xenon/graphics/material.h"

namespace xe {

	struct ModelNode {
		uint16_t parent;
		uint8_t primitiveCount;
		// NOTE: The "indices" index for the primitive is the sum of previous nodes primitiveCount
		// TODO: Evaluate if adding the index directly improves useability
	};

	struct Model {
		// NOTE: Nodes are kept sorted in DFS hierarchic dependent order
		std::vector<ModelNode> nodes;
		
		std::vector<glm::mat4x4> localPositions;
		std::vector<size_t> primitiveIndices;

		std::vector<Primitive> primitives;
		std::vector<Material> materials;
		std::vector<PrimitiveAttributeArray> primitiveAttributes;
	};

	void destroyModel(Model* model);

	struct ModelComponent {
		Model* model = nullptr;
	};

}
