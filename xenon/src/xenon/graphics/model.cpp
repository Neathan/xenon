#include "model.h"

#include "xenon/core/log.h"
#include "xenon/graphics/model_loader.h"
#include "xenon/scene/scene.h"

#include "xenon/core/assert.h"

namespace xe {

	void destroyModel(Model* model) {
		XE_LOG_TRACE_F("MODEL: Destroying model: {}", model->metadata.path);

		for (const Primitive& primitive : model->primitives) {
			glDeleteVertexArrays(1, &primitive.vao);
			if (primitive.ebo) {
				glDeleteBuffers(1, &primitive.ebo);
			}
		}

		for (const PrimitiveAttributeArray& attributeArray : model->primitiveAttributes) {
			for (const PrimitiveAttribute& attribute : attributeArray) {
				if (attribute.vbo) {
					glDeleteBuffers(1, &attribute.vbo);
				}
			}
		}

		delete model;
	}

	void setModelComponentModel(ModelComponent& modelComponent, Model* model) {
		modelComponent.model = model;
		modelComponent.currentLocalPositions = model->localPositions;
		modelComponent.currentGlobalPositions.resize(model->localPositions.size());
		for (int i = 0; i < model->skins.size(); ++i) {
			for (int j = 0; j < model->skins[i].joints.size(); ++j) {
				modelComponent.skinJointMatrices[i].push_back(glm::mat4(1.0f));
			}
		}
	}

	bool updateAnimation(ModelComponent& modelComponent, float delta) {
		XE_ASSERT(modelComponent.animation.animationIndex != -1);

		ModelAnimation& modelAnimation = modelComponent.animation;

		modelAnimation.time += delta;

		const Model& model = *modelComponent.model;
		const Animation& animation = modelComponent.model->animations.at(modelAnimation.animationIndex);

		bool updatedValues = false;

		for (size_t i = 0; i < model.nodes.size(); ++i) {
			const ModelNode& node = model.nodes[i];
			
			if (animation.nodeAnimations.find(i) == animation.nodeAnimations.end()) {
				continue;
			}

			// Update animation
			const NodeAnimation& nodeAnimation = animation.nodeAnimations.at(i);

			// Translation animation
			if (!nodeAnimation.translation.empty()) {
				updatedValues = true;

				const AnimationTransformation<glm::vec3>& translationAnimation = nodeAnimation.translation.at(modelAnimation.animationTranslationStep);

				if (modelAnimation.time < translationAnimation.time) {
					glm::vec3 targetTranslation = glm::vec3(0, 0, 0);

					// Interpolation
					if (translationAnimation.interpolation == Interpolation::LINEAR) {
						int prevIndex = modelAnimation.animationTranslationStep - 1;
						// Get previous time and translation
						float prevTime = prevIndex >= 0 ? nodeAnimation.translation.at(prevIndex).time : 0.0f;
						glm::vec3 prevTranslation = prevIndex >= 0 ? nodeAnimation.translation.at(prevIndex).value : getTransformMatrixPosition(model.localPositions.at(i));

						float interpValue = (modelAnimation.time - prevTime) / glm::max((translationAnimation.time - prevTime), glm::epsilon<float>());
						targetTranslation = prevTranslation + (translationAnimation.value - prevTranslation) * interpValue;
					}
					else if (translationAnimation.interpolation == Interpolation::STEP) {
						targetTranslation = translationAnimation.value;
					}
					else if (translationAnimation.interpolation == Interpolation::CUBICSPLINE) {
						// TODO: Implement
						targetTranslation = translationAnimation.value;
					}

					// Set value and count up
					modelComponent.currentLocalPositions[i] = setTransformMatrixPosition(modelComponent.currentLocalPositions[i], targetTranslation);
				}

				if (modelAnimation.time >= translationAnimation.time) {
					if (modelAnimation.animationTranslationStep < nodeAnimation.translation.size() - 1) {
						++modelAnimation.animationTranslationStep;
					}
				}
			}

			// Rotation animation
			if (!nodeAnimation.rotation.empty()) {
				updatedValues = true;

				const AnimationTransformation<glm::quat>& rotationAnimation = nodeAnimation.rotation.at(modelAnimation.animationRotationStep);

				if (modelAnimation.time < rotationAnimation.time) {
					glm::quat targetRotation = glm::identity<glm::quat>();

					// Interpolation
					if (rotationAnimation.interpolation == Interpolation::LINEAR) {
						int prevIndex = modelAnimation.animationRotationStep - 1;
						// Get previous time and rotation, if first use 0 and initial rotation
						float prevTime = prevIndex >= 0 ? nodeAnimation.rotation.at(prevIndex).time : 0.0f;
						glm::quat prevRotation = prevIndex >= 0 ? nodeAnimation.rotation.at(prevIndex).value : getTransformMatrixRotation(model.localPositions.at(i));

						float interpValue = (modelAnimation.time - prevTime) / glm::max((rotationAnimation.time - prevTime), glm::epsilon<float>());
						targetRotation = glm::slerp(prevRotation, rotationAnimation.value, interpValue);
					}
					else if (rotationAnimation.interpolation == Interpolation::STEP) {
						targetRotation = rotationAnimation.value;
					}
					else if (rotationAnimation.interpolation == Interpolation::CUBICSPLINE) {
						// TODO: Implement
						targetRotation = rotationAnimation.value;
					}

					// Set value and count up
					modelComponent.currentLocalPositions[i] = setTransformMatrixRotation(modelComponent.currentLocalPositions[i], targetRotation);
				}

				if (modelAnimation.time >= rotationAnimation.time) {
					if (modelAnimation.animationRotationStep < nodeAnimation.rotation.size() - 1) {
						++modelAnimation.animationRotationStep;
					}
				}
			}

			// Scale animation
			if (!nodeAnimation.scale.empty()) {
				updatedValues = true;

				const AnimationTransformation<glm::vec3>& scaleAnimation = nodeAnimation.scale.at(modelAnimation.animationScaleStep);

				if (modelAnimation.time < scaleAnimation.time) {
					glm::vec3 targetScale = glm::vec3(0, 0, 0);

					// Interpolation
					if (scaleAnimation.interpolation == Interpolation::LINEAR) {
						int prevIndex = modelAnimation.animationScaleStep - 1;
						// Get previous time and scale
						float prevTime = prevIndex >= 0 ? nodeAnimation.scale.at(prevIndex).time : 0.0f;
						glm::vec3 prevScale = prevIndex >= 0 ? nodeAnimation.scale.at(prevIndex).value : getTransformMatrixScale(model.localPositions.at(i));

						float interpValue = (modelAnimation.time - prevTime) / glm::max((scaleAnimation.time - prevTime), glm::epsilon<float>());
						targetScale = prevScale + (scaleAnimation.value - prevScale) * interpValue;
					}
					else if (scaleAnimation.interpolation == Interpolation::STEP) {
						targetScale = scaleAnimation.value;
					}
					else if (scaleAnimation.interpolation == Interpolation::CUBICSPLINE) {
						// TODO: Implement
						targetScale = scaleAnimation.value;
					}

					// Set value and count up
					modelComponent.currentLocalPositions[i] = setTransformMatrixScale(modelComponent.currentLocalPositions[i], targetScale);
				}

				if (modelAnimation.time >= scaleAnimation.time) {
					if (modelAnimation.animationScaleStep < nodeAnimation.scale.size() - 1) {
						++modelAnimation.animationScaleStep;
					}
				}
			}

			// TODO: Weight animation (morph targets)

			// Reset animation when end time is reached
			if (modelAnimation.time >= animation.endTime) {
				modelAnimation.animationTranslationStep = 0;
				modelAnimation.animationRotationStep = 0;
				modelAnimation.animationScaleStep = 0;
				modelAnimation.animationWeightStep = 0;
				modelAnimation.time -= animation.endTime;
			}
		}

		return updatedValues;
	}

	void updateInstanceTransformation(ModelComponent& modelComponent, const glm::mat4& transform) {
		XE_ASSERT(modelComponent.model);

		const Model& model = *modelComponent.model;

		for (size_t i = 0; i < model.nodes.size(); ++i) {
			const ModelNode& node = model.nodes[i];

			// Update global positions
			const glm::mat4& parentMatrix = node.parent == -1 ? glm::mat4(1.0f) : modelComponent.currentGlobalPositions[node.parent];
			modelComponent.currentGlobalPositions[i] = parentMatrix * modelComponent.currentLocalPositions[i];
			
			// Update joint matrices
			if (node.skin != -1) {
				const Skin& skin = model.skins[node.skin];

				for (int jointIndex = 0; jointIndex < skin.joints.size(); ++jointIndex) {
					modelComponent.skinJointMatrices[node.skin][jointIndex] = modelComponent.currentGlobalPositions[skin.jointNodes[jointIndex]] * skin.inverseBindMatrices[jointIndex];
				}
			}
		}
	}

	void ModelSerializer::serialize(Asset* asset) const {
		// TODO: Implement
		XE_LOG_ERROR("Model serialization is not yet implemented.");
	}

	bool ModelSerializer::loadData(Asset** asset) const {
		const Asset* sourceAsset = *asset;
		*asset = loadModel((*asset)->metadata.path);
		copyAssetMetaRuntimeData(sourceAsset, *asset);
		return true;
	}

}
