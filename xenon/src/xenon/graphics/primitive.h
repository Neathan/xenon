#pragma once

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

	struct Primitive {
		GLuint vao;
		GLenum mode;  // Type of primitive
		GLsizei count;
		int material;

		GLuint ebo = 0;
		GLenum indexType = 0;

		// DrawArrays
		Primitive(GLuint vao, GLenum mode, GLsizei count, int material);
		// DrawElements
		Primitive(GLuint vao, GLenum mode, GLsizei count, int material, GLuint ebo, GLenum indexType);
	};

}
