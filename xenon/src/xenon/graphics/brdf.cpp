#include "brdf.h"

#include "xenon/graphics/framebuffer.h"
#include "xenon/graphics/primitives.h"

namespace xe {

	Texture* generateBRDFLUT(unsigned int width, unsigned int height) {
		// Create shader, framebuffer and load model.
		Shader* shader = loadShader("assets/shaders/brdf.vert", "assets/shaders/brdf.frag");
		Model* plane = generatePlaneModel(1, 1, GeneratorDirection::FRONT);
		Framebuffer* framebuffer = createFramebuffer(width, height);

		TextureParameters params = TextureParameters{ GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
		framebuffer->attachments.insert({ GL_COLOR_ATTACHMENT0, FramebufferAttachment{ GL_COLOR_ATTACHMENT0, TextureFormat::RGB_FLOAT, params } });

		buildFramebuffer(framebuffer);

		// Render BRDF to framebuffer texture
		bindFramebuffer(*framebuffer);
		bindShader(*shader);
		glClear(GL_COLOR_BUFFER_BIT);

		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		glViewport(0, 0, width, height);

		const Primitive& primitive = plane->primitives[0];
		glBindVertexArray(primitive.vao);
		glDrawArrays(primitive.mode, 0, primitive.count);
		glBindVertexArray(0);

		unbindShader();
		unbindFramebuffer();

		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		// Extract framebuffer texture
		Texture* texture = framebuffer->attachments.at(GL_COLOR_ATTACHMENT0).texture;

		// Delete only the framebuffer, not the texture rendered to
		glDeleteFramebuffers(1, &framebuffer->frambufferID);
		delete framebuffer;

		return texture;
	}

}
