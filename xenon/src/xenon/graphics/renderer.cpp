#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "xenon/core/log.h"
#include "xenon/graphics/primitives.h"
#include "xenon/graphics/brdf.h"

#include "xenon/core/input.h"

namespace xe {

	//----------------------------------------
	// SECTION: Renderer
	//----------------------------------------

	Renderer* createRenderer(Shader* shader, Shader* envShader) {
		Texture* brdfLUT = generateBRDFLUT(512, 512);

		return new Renderer{ shader, envShader, brdfLUT };
	}

	void destroyRenderer(Renderer* renderer) {
		delete renderer;
	}

	//----------------------------------------
	// SECTION: Renderer functions
	//----------------------------------------

	// TODO: Optimize
	void loadUsedAttributes(const Shader& shader, const PrimitiveAttributeArray& attributeArray) {
		loadInt(shader, "usingAttribTangent", attributeArray[(GLuint)PrimitiveAttributeType::TANGENT].vbo == 0);
		loadInt(shader, "usingAttribNormal", attributeArray[(GLuint)PrimitiveAttributeType::NORMAL].vbo == 0);
		loadInt(shader, "usingAttribTexCoord0", attributeArray[(GLuint)PrimitiveAttributeType::TEXCOORD_0].vbo == 0);
		//loadInt(shader, "usingAttribTexCoord1", attributeArray[(GLuint)PrimitiveAttributeType::TEXCOORD_1].vbo == 0);
		//loadInt(shader, "usingAttribColor0", attributeArray[(GLuint)PrimitiveAttributeType::COLOR_0].vbo == 0);
		//loadInt(shader, "usingAttribJoints0", attributeArray[(GLuint)PrimitiveAttributeType::JOINTS_0].vbo == 0);
		//loadInt(shader, "usingAttribWeights0", attributeArray[(GLuint)PrimitiveAttributeType::WEIGHTS_0].vbo == 0);
	}

	void setObjectID(const Renderer& renderer, UUID id) {
		bindShader(*renderer.shader);
		loadInt(*renderer.shader, "objectID", id);
		unbindShader();
	}

	void renderModel(const Renderer& renderer, const Model& model, const glm::mat4& transform, const Camera& camera, bool ignoreMaterials) {
		size_t primitiveCounter = 0;

		std::vector<glm::mat4x4> globalPositions;
		globalPositions.reserve(model.localPositions.size());

		bindShader(*renderer.shader);
		loadMat4(*renderer.shader, "projection", camera.projection);
		loadMat4(*renderer.shader, "view", camera.inverseTransform);
		loadVec3(*renderer.shader, "camera.position", camera.transform[3]);
		loadVec3(*renderer.shader, "camera.direction", camera.transform[2]);

		for (size_t i = 0; i < model.nodes.size(); ++i) {
			const ModelNode& node = model.nodes[i];
			
			const glm::mat4& parentMatrix = i == 0 ? glm::mat4(1.0f) : globalPositions[node.parent];
			globalPositions.push_back(parentMatrix * model.localPositions[i]);
			loadMat4(*renderer.shader, "transform", transform * globalPositions[i]);

			// pii = primitiveIndicesIndex
			for (size_t pii = primitiveCounter; pii < primitiveCounter + node.primitiveCount; ++pii) {
				const Primitive& primitive = model.primitives[model.primitiveIndices[pii]];

				loadUsedAttributes(*renderer.shader, model.primitiveAttributes[model.primitiveIndices[pii]]);

				glBindVertexArray(primitive.vao);
				if (!ignoreMaterials) {
					if (primitive.material >= 0) {
						loadMaterial(*renderer.shader, model.materials[primitive.material]);
					}
					else {
						// TODO: Default material
						loadMaterial(*renderer.shader, Material());
					}
				}

				// Render primitive
				// Check if indexed
				if (primitive.ebo != 0) {
					glDrawElements(primitive.mode, primitive.count, primitive.indexType, 0);
				}
				else {
					glDrawArrays(primitive.mode, 0, primitive.count);
				}
			}
			glBindVertexArray(0);

			primitiveCounter += node.primitiveCount;
		}

		unbindShader();
	}

	void renderEnvironment(Renderer* renderer, const Environment& environment, const Camera& camera) {
		if (!renderer->envCubeModel) {
			renderer->envCubeModel = generateCubeModel(glm::vec3(1.0f));
		}

		bindShader(*renderer->envShader);
		loadMat4(*renderer->envShader, "projection", camera.projection);
		loadMat4(*renderer->envShader, "view", camera.inverseTransform);

		glBindTextureUnit(0, environment.environmentCubemap->textureID);

		glDisable(GL_CULL_FACE);
		const Primitive& primitive = renderer->envCubeModel->primitives[0];
		glBindVertexArray(primitive.vao);
		glDrawArrays(primitive.mode, 0, primitive.count);
		glBindVertexArray(0);
		glEnable(GL_CULL_FACE);

		glBindTextureUnit(0, 0);
		unbindShader();
	}

	void renderGrid(Shader* shader, Model* model, const Camera& camera) {
		bindShader(*shader);
		loadMat4(*shader, "projection", camera.projection);
		loadMat4(*shader, "view", camera.inverseTransform);
		loadFloat(*shader, "near", camera.near);
		loadFloat(*shader, "far", camera.far);

		const Primitive& primitive = model->primitives[0];
		glBindVertexArray(primitive.vao);
		glDrawArrays(primitive.mode, 0, primitive.count);
		glBindVertexArray(0);

		unbindShader();
	}


	//----------------------------------------
	// SECTION: Framebuffer renderer
	//----------------------------------------

	FramebufferRenderer* createFramebufferRenderer(Shader* shader) {
		FramebufferRenderer* renderer = new FramebufferRenderer();
		renderer->shader = shader;
		renderer->planeModel = generatePlaneModel(1.0f, 1.0f, GeneratorDirection::FRONT);
		return renderer;
	}

	void destroyFramebufferRenderer(FramebufferRenderer* renderer) {
		destroyModel(renderer->planeModel);
		delete renderer;
	}


	//----------------------------------------
	// SECTION: Framebuffer renderer functions
	//----------------------------------------

	void renderFramebufferToScreen(const FramebufferRenderer& renderer, const Framebuffer& framebuffer) {
		bindShader(*renderer.shader);

		glBindTextureUnit(0, framebuffer.attachments.at(GL_COLOR_ATTACHMENT0).texture->textureID);

		const Primitive& primitive = renderer.planeModel->primitives[0];

		glBindVertexArray(primitive.vao);
		glDrawArrays(primitive.mode, 0, primitive.count);
		glBindVertexArray(0);

		glBindTextureUnit(0, 0);

		unbindShader();
	}

}

