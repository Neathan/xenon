#include "model_loader.h"

#include <queue>

#include <tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>

#include "xenon/core/log.h"
#include "xenon/core/assert.h"
#include "xenon/graphics/material.h"

#include "xenon/core/asset_manager.h"

namespace xe {

	glm::mat4 getNodeTransform(const tinygltf::Node& node) {
		if (node.matrix.size() == 16) {
			return glm::make_mat4(node.matrix.data());
		}

		glm::mat4 result = glm::mat4(1.0f);
		// TODO: Optimize, check if casting is the right choice here
		if (node.translation.size() == 3) {
			result = glm::translate(result, (glm::vec3)glm::make_vec3(node.translation.data()));
		}
		if (node.rotation.size() == 3) {
			result *= glm::mat4_cast((glm::quat)glm::make_quat(node.rotation.data()));
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
				tinygltf::Accessor accessor = model.accessors[accessorIndex];
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
				//if (attribute.compare("JOINTS_0") == 0) type = PrimitiveAttributeType::JOINTS_0;
				//if (attribute.compare("WEIGHTS_0") == 0) type = PrimitiveAttributeType::WEIGHTS_0;

				if (type != PrimitiveAttributeType::INVALID) {
					GLuint vaa = (GLuint)type;
					glEnableVertexArrayAttrib(vao, vaa);
					glVertexArrayAttribFormat(vao, vaa, size, accessor.componentType, accessor.normalized, 0);
					glVertexArrayAttribBinding(vao, vaa, vaa);
					glVertexArrayVertexBuffer(vao, vaa, vbo, (GLintptr)((char*)NULL + accessor.byteOffset), byteStride);

					// NOTE: Casting size_t to GLsizei is neccesary to comply with the limit set by OpenGL
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

				// NOTE: Casting size_t to GLsizei is neccesary to comply with the limit set by OpenGL
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
		std::map<size_t, GLuint> bufferVBOs;
		for (size_t i = 0; i < gltfModel.bufferViews.size(); ++i) {
			const auto& bufferView = gltfModel.bufferViews.at(i);

			// TODO: Make proper use of bufferView.target

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
		model->nodes.push_back(ModelNode{ 0, 0 });
		model->localPositions.emplace_back(glm::mat4x4(1.0f));

		// This struct is needed for itterative itteration of the node hierarchy
		struct QueuedNode {
			int parent;
			int node;
		};
		std::queue<QueuedNode> queuedNodes;

		// Queue all root nodes of the scene
		const auto& sceneNodes = gltfModel.scenes[gltfModel.defaultScene].nodes;
		for (size_t i = 0; i < sceneNodes.size(); ++i) {
			const auto& rootNode = sceneNodes[i];

			queuedNodes.push(QueuedNode{0, rootNode});
		}
		
		
		// Process nodes
		uint16_t currentIndex = 1;
		size_t currentPrimitiveIndex = 0;

		std::vector<glm::mat4x4> globalPositions;
		globalPositions.emplace_back(glm::mat4x4(1.0f));

		while (!queuedNodes.empty()) {
			const QueuedNode& currentNode = queuedNodes.front();
			const tinygltf::Node& node = gltfModel.nodes[currentNode.node];
			
			// Process node
			// Add local position
			glm::mat4 localPosition = getNodeTransform(node);
			model->localPositions.push_back(localPosition);
			
			const glm::mat4& parentMatrix = globalPositions[currentNode.parent];
			glm::mat4 globalPosition = parentMatrix * localPosition;
			globalPositions.push_back(globalPosition);

			// Check if node has valid mesh
			uint8_t primitiveCount = 0;
			if (node.mesh >= 0 && node.mesh < gltfModel.meshes.size()) {
				size_t count = processPrimitives(gltfModel, gltfModel.meshes[node.mesh], bufferVBOs, path, currentPrimitiveIndex, globalPosition, model);
				primitiveCount += count;
				currentPrimitiveIndex += count;
			}

			// Queue children
			for (const auto& child : node.children) {
				queuedNodes.push(QueuedNode{ currentIndex, child });
			}

			// Add node
			model->nodes.push_back(ModelNode{ (uint16_t)currentNode.parent, primitiveCount });

			++currentIndex;
			queuedNodes.pop();
		}

		XE_LOG_TRACE_F("MODEL_LOADER: Loaded model: {}", path);
		return model;
	}

}
