#include "primitive.h"

namespace xe {

	BoundingBox BoundingBox::operator* (glm::mat4x4 matrix) {
		BoundingBox bounds;
		bounds.min = matrix * glm::vec4(min, 1);
		bounds.max = matrix * glm::vec4(max, 1);
		return bounds;
	}

	BoundingBox BoundingBox::operator+ (const BoundingBox& other) {
		BoundingBox bounds;
		bounds.min = glm::min(min, other.min);
		bounds.max = glm::max(max, other.max);
		return bounds;
	}

	Primitive::Primitive(GLuint vao, GLenum mode, GLsizei count, int material, const BoundingBox& bounds)
		: vao(vao), mode(mode), count(count), material(material), bounds(bounds) {}

	Primitive::Primitive(GLuint vao, GLenum mode, GLsizei count, int material, const BoundingBox& bounds, GLuint ebo, GLenum indexType)
		: vao(vao), mode(mode), count(count), material(material), bounds(bounds), ebo(ebo), indexType(indexType) {}

}

