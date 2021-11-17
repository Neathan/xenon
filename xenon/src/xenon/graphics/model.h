#pragma once

#include <cstdint>
#include <vector>

#include <glm/mat4x4.hpp>

#include "xenon/graphics/primitive.h"
#include "xenon/graphics/material.h"
#include "xenon/graphics/animation.h"
#include "xenon/graphics/skin.h"

#include "xenon/core/asset.h"

namespace xe {

	struct ModelNode {
		int parent;
		// NOTE: The primitiveIndices index for the primitive is the sum of previous nodes primitiveCount
		int primitiveCount;
		int skin;
		std::string name;
	};

	struct Model : Asset {
		// NOTE: Nodes are kept sorted in DFS hierarchic dependent order
		std::vector<ModelNode> nodes;
		
		std::vector<glm::mat4> localPositions;
		std::vector<int> primitiveIndices;

		std::vector<Primitive> primitives;
		std::vector<Material> materials;
		std::vector<PrimitiveAttributeArray> primitiveAttributes;

		std::vector<Animation> animations;

		std::vector<Skin> skins;

		BoundingBox bounds;
	};

	void destroyModel(Model* model);

	struct ModelAnimation {
		int animationIndex = -1;
		float time = 0.0f;

		bool isDirty = false;

		int animationWeightStep = 0;
		int animationTranslationStep = 0;
		int animationRotationStep = 0;
		int animationScaleStep = 0;
	};

	struct ModelComponent {
		Model* model = nullptr;
		std::vector<glm::mat4> currentLocalPositions;
		std::vector<glm::mat4> currentGlobalPositions;
		std::map<int, std::vector<glm::mat4>> skinJointMatrices;

		ModelAnimation animation;
		bool wireframe = false;
	};

	void setModelComponentModel(ModelComponent& modelComponent, Model* model);

	bool updateAnimation(ModelComponent& modelComponent, bool isPlaying, float delta);

	void updateInstanceTransformation(ModelComponent& modelComponent, const glm::mat4& transform);

}
