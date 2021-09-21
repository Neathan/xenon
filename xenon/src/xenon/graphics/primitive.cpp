#include "primitive.h"

namespace xe {

	Primitive::Primitive(GLuint vao, GLenum mode, GLsizei count, int material)
		: vao(vao), mode(mode), count(count), material(material) {}

	Primitive::Primitive(GLuint vao, GLenum mode, GLsizei count, int material, GLuint ebo, GLenum indexType)
		: vao(vao), mode(mode), count(count), material(material), ebo(ebo), indexType(indexType) {}

}

