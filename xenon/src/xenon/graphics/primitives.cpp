#include "primitives.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include "xenon/core/log.h"

namespace xe {

	Model* generatePlaneModel(float width, float height, GeneratorDirection facing) {
		static GLuint planeVAO = 0;
		static GLuint planeVBO = 0;

		// Load planeData
		if (planeVAO == 0) {
			// Positions and texture coordinates
			const std::array<float, 30> planeData = {
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,  // top left
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom left
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,  // top right

				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,  // top right
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom left
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f   // bottom right
			};

			glCreateVertexArrays(1, &planeVAO);

			glCreateBuffers(1, &planeVBO);
			glNamedBufferStorage(planeVBO, sizeof(planeData), planeData.data(), 0);

			GLuint vaa = (GLuint)PrimitiveAttributeType::POSITION;
			glEnableVertexArrayAttrib(planeVAO, 0);
			glVertexArrayAttribFormat(planeVAO, vaa, 3, GL_FLOAT, false, 0);
			glVertexArrayAttribBinding(planeVAO, vaa, 0);

			vaa = (GLuint)PrimitiveAttributeType::TEXCOORD_0;
			glEnableVertexArrayAttrib(planeVAO, 3);
			glVertexArrayAttribFormat(planeVAO, vaa, 2, GL_FLOAT, false, sizeof(float) * 3);
			glVertexArrayAttribBinding(planeVAO, vaa, 0);

			glVertexArrayVertexBuffer(planeVAO, 0, planeVBO, 0, sizeof(float) * 5);

			XE_LOG_TRACE("PRIMITIVES: Loaded plane primitive date to VAO");
		}

		static std::unordered_map<GeneratorDirection, glm::mat4> facingTransforms;
		if (facingTransforms.empty()) {
			facingTransforms[GeneratorDirection::FRONT] =	glm::mat4(1.0f);
			facingTransforms[GeneratorDirection::BACK]  =	glm::rotate(glm::mat4(1.0f),  glm::pi     <float>(), glm::vec3(1.0f, 0.0f, 0.0f));
			facingTransforms[GeneratorDirection::UP]    =	glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
			facingTransforms[GeneratorDirection::DOWN]  =	glm::rotate(glm::mat4(1.0f),  glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
			facingTransforms[GeneratorDirection::LEFT]  =	glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
			facingTransforms[GeneratorDirection::RIGHT] =	glm::rotate(glm::mat4(1.0f),  glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		// Create facing and scale transform
		glm::mat4 transform = glm::mat4(1.0f);
		transform = facingTransforms[facing] * glm::scale(transform, glm::vec3(width, height, 1.0f));

		// Create model
		Model* model = new Model();

		Primitive primitive = Primitive(planeVAO, GL_TRIANGLES, 6, 0);
		PrimitiveAttributeArray attributeArray;
		attributeArray[(uint8_t)PrimitiveAttributeType::POSITION] = PrimitiveAttribute{ planeVBO, 6 };

		model->nodes.push_back(ModelNode{ 0, 1 });
		model->localPositions.push_back(transform);
		model->primitiveIndices.push_back(0);
		model->primitives.push_back(primitive);
		model->materials.push_back(Material());
		model->primitiveAttributes.push_back(attributeArray);

		return model;
	}

	Model* generateCubeModel(glm::vec3 size) {
		static GLuint cubeVAO = 0;
		static GLuint cubeVBO = 0;

		// Load cubeData
		if (cubeVAO == 0) {
			// Positions and texture coordinates
			const std::array<float, 288> cubeData = {
				// Position (x, y, z), Normal (x, y, z), UV (x, y)
				// Back face
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // Bottom left
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // Top right
				 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // Bottom right
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // Top right
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // Bottom left
				-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // Top left
				// Front face
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // Bottom left
				 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // Bottom right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // Top right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // Top right
				-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // Top left
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // Bottom left
				// Left face
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top right
				-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // Top left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom left
				-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // Bottom right
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top right
				// Right face
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top left
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom right
				 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // Top right
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom right
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top left
				 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // Bottom left
				// Bottom face
				-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // Top right
				 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // Top left
				 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // Bottom left
				 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // Bottom left
				-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // Bottom right
				-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // Top right
				// Top face
				-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // Top left
				 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // Bottom right
				 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // Top right
				 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // Bottom right
				-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // Top left
				-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // Bottom left
			};

			glCreateVertexArrays(1, &cubeVAO);

			glCreateBuffers(1, &cubeVBO);
			glNamedBufferStorage(cubeVBO, sizeof(cubeData), cubeData.data(), 0);

			GLuint vaa = (GLuint)PrimitiveAttributeType::POSITION;
			glEnableVertexArrayAttrib(cubeVAO, vaa);
			glVertexArrayAttribFormat(cubeVAO, vaa, 3, GL_FLOAT, false, 0);
			glVertexArrayAttribBinding(cubeVAO, vaa, 0);

			vaa = (GLuint)PrimitiveAttributeType::NORMAL;
			glEnableVertexArrayAttrib(cubeVAO, vaa);
			glVertexArrayAttribFormat(cubeVAO, vaa, 3, GL_FLOAT, false, sizeof(float) * 3);
			glVertexArrayAttribBinding(cubeVAO, vaa, 0);

			vaa = (GLuint)PrimitiveAttributeType::TEXCOORD_0;
			glEnableVertexArrayAttrib(cubeVAO, vaa);
			glVertexArrayAttribFormat(cubeVAO, vaa, 2, GL_FLOAT, false, sizeof(float) * 3 * 2);
			glVertexArrayAttribBinding(cubeVAO, vaa, 0);

			glVertexArrayVertexBuffer(cubeVAO, 0, cubeVBO, 0, sizeof(float) * 8);

			XE_LOG_TRACE("PRIMITIVES: Loaded cube primitive date to VAO");
		}

		// Create scale transform
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::scale(transform, size);

		// Create model
		Model* model = new Model();

		Primitive primitive = Primitive(cubeVAO, GL_TRIANGLES, 36, 0);
		PrimitiveAttributeArray attributeArray;
		attributeArray[(uint8_t)PrimitiveAttributeType::POSITION] = PrimitiveAttribute{ cubeVBO, 36 };

		model->nodes.push_back(ModelNode{ 0, 1 });
		model->localPositions.push_back(transform);
		model->primitiveIndices.push_back(0);
		model->primitives.push_back(primitive);
		model->materials.push_back(Material());
		model->primitiveAttributes.push_back(attributeArray);

		return model;
	}

}

