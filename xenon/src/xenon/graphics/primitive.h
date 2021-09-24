#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>

#include <glad/gl.h>

namespace xe {

	// This should be the same value as the number of valid primitive attribute types
	#define XE_PRIMITIVE_ATTRIBUTE_TYPES 4

	enum class PrimitiveAttributeType : uint8_t {
		POSITION		= 0,
		TANGENT			= 1,
		NORMAL			= 2,
		TEXCOORD_0		= 3,
		// TEXCOORD_1	= 4,
		// COLOR_0		= 5,
		// JOINTS_0		= 6,
		// WEIGHTS_0	= 7,
		INVALID			= UINT8_MAX
	};

	struct PrimitiveAttribute {
		GLuint vbo = 0;
		GLsizei count = 0;
	};

	typedef std::array<PrimitiveAttribute, XE_PRIMITIVE_ATTRIBUTE_TYPES> PrimitiveAttributeArray;

	struct BoundingBox {
		glm::vec3 min;
		glm::vec3 max;

		BoundingBox operator* (glm::mat4x4 matrix);
		BoundingBox operator+ (const BoundingBox& other);
	};

	struct Primitive {
		GLuint vao;
		GLenum mode;  // Type of primitive
		GLsizei count;
		int material;
		BoundingBox bounds;

		GLuint ebo = 0;
		GLenum indexType = 0;

		// DrawArrays
		Primitive(GLuint vao, GLenum mode, GLsizei count, int material, const BoundingBox& bounds);
		// DrawElements
		Primitive(GLuint vao, GLenum mode, GLsizei count, int material, const BoundingBox& bounds, GLuint ebo, GLenum indexType);
	};

}
