#pragma once

#include <map>
#include <vector>

#include <glad/gl.h>

#include "xenon/graphics/texture.h"
#include "xenon/graphics/shader.h"

namespace xe {

	struct FramebufferAttachment {
		GLenum target;
		TextureFormat format;
		TextureParameters textureParams;
		Texture* texture = nullptr;
	};

	enum class FramebufferStatus {
		UNINITIALIZED,
		INCOMPLETE,
		COMPLETE
	};

	typedef std::pair<GLenum, FramebufferAttachment> FramebufferAttachmentPair;

	struct Framebuffer {
		GLuint width, height;
		int samples;
		GLuint frambufferID = 0;
		FramebufferStatus status = FramebufferStatus::UNINITIALIZED;
		std::map<GLenum, FramebufferAttachment> attachments;
		std::vector<GLenum> colorBuffers;
	};

	Framebuffer* createFramebuffer(unsigned int width, unsigned int height, int samples = 1);
	void destroyFramebuffer(Framebuffer* framebuffer);

	bool buildFramebuffer(Framebuffer* framebuffer);

	void bindFramebuffer(const Framebuffer& framebuffer);
	void unbindFramebuffer();

	void clearFramebuffer(const Framebuffer& framebuffer, const Shader& shader);
	void updateFramebufferSize(Framebuffer* framebuffer, unsigned int width, unsigned int height);
	void blitFramebuffers(Framebuffer* source, Framebuffer* target);

	typedef enum class DefaultFramebufferAttachmentType {
		COLOR,
		DEPTH,
		INTEGER
	} DefaultAttachmentType;

	FramebufferAttachmentPair createDefaultFramebufferAttachment(DefaultAttachmentType type, GLuint target = 0);

}
