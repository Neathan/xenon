#include "model_loader.h"

#include <queue>

#include <tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "xenon/core/log.h"
#include "xenon/core/assert.h"
#include "xenon/graphics/material.h"

#include "xenon/core/asset_manager.h"

namespace xe {

	glm::mat4 getNodeTransform(const tinygltf::Node& node) {
		if (node.matrix.size() == 16) {
			return glm::make_mat4(node.matrix.data());
		}
		// TODO: Optimize, check if casting is the right choice here

		glm::mat4 result = glm::mat4(1.0f);
		if (node.translation.size() == 3) {
			result = glm::translate(result, (glm::vec3)glm::make_vec3(node.translation.data()));
		}
		if (node.rotation.size() == 4) {
			glm::vec4 inputValue = (glm::vec4)glm::make_vec4((node.rotation.data()));
			result *= glm::toMat4(glm::quat(inputValue.w, inputValue.x, inputValue.y, inputValue.z));
		}
		if (node.scale.size() == 3) {
			result = glm::scale(result, (glm::vec3)glm::make_vec3(node.scale.data()));
		}

		return result;
	}

	template<typename T>
	Texture* loadTextureIfExists(AssetManager* manager, const tinygltf::Model& model, const T& info, const std::string& path, bool srgb = false) {
		if (info.index == -1) {
			return nullptr;
		}

		const tinygltf::Texture& texture = model.textures[info.index];
		const tinygltf::Image& image = model.images[texture.source];

		TextureParameters textureParameters;
		if (texture.sampler >= 0) {
			const tinygltf::Sampler& sampler = model.samplers[texture.sampler];
			textureParameters.minFilter = sampler.minFilter == -1 ? GL_LINEAR : sampler.minFilter;
			textureParameters.magFilter = sampler.magFilter == -1 ? GL_LINEAR : sampler.magFilter;
			textureParameters.wrapS = sampler.wrapS;
			textureParameters.wrapT = sampler.wrapT;
		}

		TextureFormat format = channelsToFormat(image.component, srgb);  // TODO: Check if this is the valid way to do sRGB 

		// TODO: Add multi-texture-coordinate support: info.texCoord
		// TODO: Improve texture signature
		// std::string signature = TEXTURE_INTERNAL_SIGNATURE + std::to_string(info.index) + ":" + std::to_string((int)type) + ":" + path;
		return createInternalTextureAsset(manager, path, std::to_string(info.index), image.image.data(), image.width, image.height, image.component, format, textureParameters);
	}

	size_t processPrimitives(const tinygltf::Model& model,
		const tinygltf::Mesh& mesh,
		const std::map<size_t, GLuint>& bufferVBOs,
		const std::string& path,
		const size_t basePrimitiveIndex,
		const glm::mat4& globalPosition,
		Model* outputModel) {

		size_t primitiveCount = 0;

		for (const auto& primitive : mesh.primitives) {
			GLuint vao;
			glCreateVertexArrays(1, &vao);

			PrimitiveAttributeArray primitiveAttributeArray;

			BoundingBox primitiveBounds;

			// Process attributes
			for (const auto& [attribute, accessorIndex] : primitive.attributes) {
				const auto& accessor = model.accessors[accessorIndex];
				int byteStride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
				int size = accessor.type == TINYGLTF_TYPE_SCALAR ? 1 : accessor.type;
				GLuint vbo = bufferVBOs.at(accessor.bufferView);

				PrimitiveAttributeType type = PrimitiveAttributeType::INVALID;
				if (attribute.compare("POSITION") == 0) {
					type = PrimitiveAttributeType::POSITION;
					primitiveBounds = BoundingBox {
						globalPosition * glm::vec4(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2], 1),
						globalPosition * glm::vec4(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2], 1)
					};
				}
				if (attribute.compare("TANGENT") == 0) type = PrimitiveAttributeType::TANGENT;
				if (attribute.compare("NORMAL") == 0) type = PrimitiveAttributeType::NORMAL;
				if (attribute.compare("TEXCOORD_0") == 0) type = PrimitiveAttributeType::TEXCOORD_0;
				// TODO: Implement additional attributes
				//if (attribute.compare("TEXCOORD_1") == 0) type = PrimitiveAttributeType::TEXCOORD_1;
				//if (attribute.compare("COLOR_0") == 0) type = PrimitiveAttributeType::COLOR_0;
				if (attribute.compare("JOINTS_0") == 0) type = PrimitiveAttributeType::JOINTS_0;
				if (attribute.compare("WEIGHTS_0") == 0) type = PrimitiveAttributeType::WEIGHTS_0;

				if (type != PrimitiveAttributeType::INVALID) {
					GLuint vaa = (GLuint)type;
					glEnableVertexArrayAttrib(vao, vaa);
					glVertexArrayAttribFormat(vao, vaa, size, accessor.componentType, accessor.normalized, 0);
					glVertexArrayAttribBinding(vao, vaa, vaa);
					glVertexArrayVertexBuffer(vao, vaa, vbo, (GLintptr)((char*)NULL + accessor.byteOffset), byteStride);

					// NOTE: Casting size_t to GLsizei is necessary to comply with the limit set by OpenGL
					primitiveAttributeArray[(uint8_t)type] = PrimitiveAttribute{ vbo, (GLsizei)accessor.count };
				}
				else {
					XE_LOG_WARN_F("MODEL_LOADER: Model contains primitive with unsupported attribute: {}", attribute);
				}
			}
			// Add primitive attributes array
			outputModel->primitiveAttributes.push_back(primitiveAttributeArray);
			
			// TODO: Fix issue with bounding box being initialized as zero,zero
			outputModel->bounds = outputModel->bounds + primitiveBounds;

			// Check if primitive is indexed or not
			if (primitive.indices >= 0) {
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				GLuint ebo = bufferVBOs.at(indexAccessor.bufferView);
				glVertexArrayElementBuffer(vao, ebo);
				
				// NOTE: Casting size_t to GLsizei is necessary to comply with the limit set by OpenGL
				outputModel->primitives.push_back(Primitive{ vao, (GLenum)primitive.mode, (GLsizei)indexAccessor.count, primitive.material, primitiveBounds, ebo, (GLenum)indexAccessor.componentType });
			}
			else {
				// Check for missing position attribute on non-index primitive
				// TODO: This could be generalized, specs says:
				// "When the indices property is not defined, GL's drawArrays function
				//  should be used with a count equal to the count property of any of
				//  the accessors referenced by the attributes property (they are all
				//  equal for a given primitive)."
				const auto& attribute = primitiveAttributeArray[(uint8_t)PrimitiveAttributeType::POSITION];
				XE_ASSERT(attribute.vbo != 0);
				outputModel->primitives.push_back(Primitive{ vao, (GLenum)primitive.mode, attribute.count, primitive.material, primitiveBounds });
			}
			// Add primitive index
			outputModel->primitiveIndices.push_back(basePrimitiveIndex + primitiveCount);

			// Increment counter
			++primitiveCount;
		}

		return primitiveCount;
	}

	Model* loadModel(const std::string& path) {
		tinygltf::TinyGLTF loader;
		tinygltf::Model gltfModel;
		std::string err, warn;
		bool result = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);

		if (!warn.empty()) {
			XE_LOG_WARN_F("MODEL_LOADER: GLTF Warning: {}", warn);
			return nullptr;
		}
		if (!err.empty()) {
			XE_LOG_WARN_F("MODEL_LOADER: GLTF Error: {}", err);
			return nullptr;
		}

		if (!result) {
			XE_LOG_ERROR_F("MODEL_LOADER: Failed to parse GLTF file at: {}", path);
			return nullptr;
		}

		// Buffer all primitive buffers
		// TODO: Don't store animation data or other data not used by GPU.
		std::map<size_t, GLuint> bufferVBOs;
		for (size_t i = 0; i < gltfModel.bufferViews.size(); ++i) {
			const auto& bufferView = gltfModel.bufferViews.at(i);

			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			GLuint vbo;
			glCreateBuffers(1, &vbo);
			glNamedBufferStorage(vbo, bufferView.byteLength, buffer.data.data() + bufferView.byteOffset, 0);
			bufferVBOs.emplace(i, vbo);
		}

		// TODO: Add support for multiple scenes
		Model* model = new Model();

		AssetManager* manager = getAssetManager();


		// Process materials
		for (const auto& pMaterial : gltfModel.materials) {
			Material material;
			material.name = pMaterial.name;

			material.pbrMetallicRoughness.baseColorFactor = glm::make_vec4(pMaterial.pbrMetallicRoughness.baseColorFactor.data());
			material.pbrMetallicRoughness.baseColorTexture = loadTextureIfExists<tinygltf::TextureInfo>(manager, gltfModel, pMaterial.pbrMetallicRoughness.baseColorTexture, path, true);
			material.pbrMetallicRoughness.metallicFactor = pMaterial.pbrMetallicRoughness.metallicFactor;
			material.pbrMetallicRoughness.roughnessFactor = pMaterial.pbrMetallicRoughness.roughnessFactor;
			material.pbrMetallicRoughness.metallicRoughnessTexture = loadTextureIfExists<tinygltf::TextureInfo>(manager, gltfModel, pMaterial.pbrMetallicRoughness.metallicRoughnessTexture, path);

			material.normalTexture = loadTextureIfExists<tinygltf::NormalTextureInfo>(manager, gltfModel, pMaterial.normalTexture, path);

			material.occlusionTexture = loadTextureIfExists<tinygltf::OcclusionTextureInfo>(manager, gltfModel, pMaterial.occlusionTexture, path);

			material.emissiveTexture = loadTextureIfExists<tinygltf::TextureInfo>(manager, gltfModel, pMaterial.emissiveTexture, path);
			material.emissiveFactor = glm::make_vec3(pMaterial.emissiveFactor.data());

			if (pMaterial.alphaMode.compare("OPAQUE") == 0) {
				material.alphaMode = AlphaMode::OPAQUE_IGNORE;
			}
			else if (pMaterial.alphaMode.compare("MASK") == 0) {
				material.alphaMode = AlphaMode::MASK;
			}
			else if (pMaterial.alphaMode.compare("BLEND") == 0) {
				material.alphaMode = AlphaMode::BLEND;
			}

			material.alphaCutoff = pMaterial.alphaCutoff;
			material.doubleSided = pMaterial.doubleSided;

			model->materials.push_back(material);
		}


		// Root node (needed since a scene can contain multiple root nodes)
		// model->nodes.push_back(ModelNode{ 0, 0 });
		// model->localPositions.emplace_back(glm::mat4x4(1.0f));

		// This struct is needed for iterative iteration of the node hierarchy
		struct QueuedNode {
			int parent;
			int node;
		};
		std::queue<QueuedNode> queuedNodes;

		// Mappings for converting original index to resulting index
		std::map<int, int> nodeMapping;

		std::vector<glm::mat4> globalPositions;
		//globalPositions.push_back(glm::mat4(1.0f));


		// Process root nodes sequentially
		int currentIndex = 0;
		size_t currentPrimitiveIndex = 0;

		int currentSkinIndex = 0;

		const auto& sceneNodes = gltfModel.scenes[gltfModel.defaultScene == -1 ? 0 : gltfModel.defaultScene].nodes;
		for (size_t i = 0; i < sceneNodes.size(); ++i) {
			const auto& rootNode = sceneNodes[i];

			queuedNodes.push(QueuedNode{-1, rootNode});
			globalPositions.emplace_back(getNodeTransform(gltfModel.nodes[rootNode]));

			// Clear current node queue (root node-tree)
			while (!queuedNodes.empty()) {
				const QueuedNode& currentNode = queuedNodes.front();
				const tinygltf::Node& node = gltfModel.nodes[currentNode.node];

				nodeMapping[currentNode.node] = currentIndex;

				// Process node
				// Add local position
				glm::mat4 localPosition = getNodeTransform(node);
				model->localPositions.push_back(localPosition);

				const glm::mat4& parentMatrix = currentNode.parent != -1 ? globalPositions[currentNode.parent] : glm::mat4(1.0f);
				glm::mat4 globalPosition = parentMatrix * localPosition;
				globalPositions.push_back(globalPosition);

				// Check if node has valid mesh
				int primitiveCount = 0;
				int nodeSkin = -1;
				if (node.mesh >= 0 && node.mesh < gltfModel.meshes.size()) {
					// Add primitives
					size_t count = processPrimitives(gltfModel, gltfModel.meshes[node.mesh], bufferVBOs, path, currentPrimitiveIndex, globalPosition, model);
					primitiveCount += count;
					currentPrimitiveIndex += count;

					if (node.skin >= 0) {
						// Add skin
						tinygltf::Skin& gltfSkin = gltfModel.skins[node.skin];
						Skin skin;
						skin.skeleton = gltfSkin.skeleton;
						skin.joints = gltfSkin.joints;
						skin.name = gltfSkin.name;
						// NOTE: jointNodes are added after all nodes have been processed

						// Get skin inverse bind matrices accessor data
						const auto& accessor = gltfModel.accessors[gltfSkin.inverseBindMatrices];
						const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
						const unsigned char* basePointer = gltfModel.buffers[bufferView.buffer].data.data() + accessor.byteOffset + bufferView.byteOffset;
						int byteStride = accessor.ByteStride(bufferView);

						for (size_t i = 0; i < accessor.count; ++i) {
							glm::mat4 value = glm::make_mat4((float*)(basePointer + byteStride * i));
							skin.inverseBindMatrices.push_back(value);
						}

						// Add the skin to the model
						model->skins.push_back(skin);
						
						nodeSkin = currentSkinIndex;
						++currentSkinIndex;
					}
				}

				// Queue children
				for (const auto& child : node.children) {
					queuedNodes.push(QueuedNode{ currentIndex, child });
				}

				// Add node
				model->nodes.push_back(ModelNode{ currentNode.parent, primitiveCount, nodeSkin, node.name });

				++currentIndex;
				queuedNodes.pop();
			}
		}
		// Add joint nodes to skins (remapped joints)
		for (Skin& skin : model->skins) {
			for (int& joint : skin.joints) {
				skin.jointNodes.push_back(nodeMapping[joint]);
			}
			
			if (skin.skeleton != -1) {
				skin.skeleton = nodeMapping.at(skin.skeleton);
			}

		}

		// Process animations
		for (const auto& gltfAnimation : gltfModel.animations) {
			Animation animation;
			animation.name = gltfAnimation.name;

			for (const auto& channel : gltfAnimation.channels) {
				// Get common data
				int node = nodeMapping.at(channel.target_node); // Get node and convert to current index
				const auto& sampler = gltfAnimation.samplers.at(channel.sampler);
				Interpolation interpolation = getInterpolation(sampler.interpolation);

				// Find input base pointer and stride
				const auto& inputAccessor = gltfModel.accessors[sampler.input];
				const auto& inputBufferView = gltfModel.bufferViews[inputAccessor.bufferView];
				const unsigned char* inputBasePointer = gltfModel.buffers[inputBufferView.buffer].data.data() + inputAccessor.byteOffset + inputBufferView.byteOffset;
				int inputByteStride = inputAccessor.ByteStride(inputBufferView);

				// Find output base pointer and stride
				const auto& outputAccessor = gltfModel.accessors[sampler.output];
				const auto& outputBufferView = gltfModel.bufferViews[outputAccessor.bufferView];
				const unsigned char* outputBasePointer = gltfModel.buffers[outputBufferView.buffer].data.data() + outputAccessor.byteOffset + outputBufferView.byteOffset;
				int outputByteStride = outputAccessor.ByteStride(outputBufferView);

				// Add all values to node animation
				for (size_t i = 0; i < inputAccessor.count; ++i) {
					float time = *(float*)(inputBasePointer + inputByteStride * i);

					NodeAnimation& nodeAnimation = animation.nodeAnimations[node];

					if (channel.target_path == "translation") {
						glm::vec3 value = glm::make_vec3((float*)(outputBasePointer + outputByteStride * i));
						nodeAnimation.translation.push_back(AnimationTransformation<glm::vec3>{ time, value, interpolation });
						if (animation.keyFrames < nodeAnimation.translation.size()) {
							animation.keyFrames = nodeAnimation.translation.size();
							if (animation.endTime < time) animation.endTime = time;
						}
					}
					else if (channel.target_path == "rotation") {
						// NOTE: Input uses X,Y,Z,W order while quaternion is W,X,Y,Z
						glm::vec4 inputValue = glm::make_vec4((float*)(outputBasePointer + outputByteStride * i));
						glm::quat value = glm::quat(inputValue.w, inputValue.x, inputValue.y, inputValue.z);
						nodeAnimation.rotation.push_back(AnimationTransformation<glm::quat>{ time, value, interpolation });
						if (animation.keyFrames < nodeAnimation.rotation.size()) {
							animation.keyFrames = nodeAnimation.rotation.size();
							if (animation.endTime < time) animation.endTime = time;
						}
					}
					else if (channel.target_path == "scale") {
						glm::vec3 value = glm::make_vec3((float*)(outputBasePointer + outputByteStride * i));
						nodeAnimation.scale.push_back(AnimationTransformation<glm::vec3>{ time, value, interpolation });
						if (animation.keyFrames < nodeAnimation.scale.size()) {
							animation.keyFrames = nodeAnimation.scale.size();
							if (animation.endTime < time) animation.endTime = time;
						}
					}
					else { // "weights" (morph targets)
						// TODO: Implement
						if (animation.keyFrames < nodeAnimation.weight.size()) {
							animation.keyFrames = nodeAnimation.weight.size();
							if (animation.endTime < time) animation.endTime = time;
						}
					}
				}
			}

			model->animations.push_back(animation);
		}

		XE_LOG_TRACE_F("MODEL_LOADER: Loaded model: {}", path);
		return model;
	}

}
