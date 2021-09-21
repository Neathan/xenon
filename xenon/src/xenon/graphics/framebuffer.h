#pragma once

#include <map>

#include <glad/gl.h>

#include "xenon/graphics/texture.h"
#include "xenon/graphics/shader.h"

namespace xe {

	struct FramebufferAttachment {
		GLenum target;
		TextureType type;
		TextureFormat format;
		TextureParameters textureParams;
		std::shared_ptr<Texture> texture = nullptr;
	};

	enum class FramebufferStatus {
		UNINITIALIZED,
		INCOMPLETE,
		COMPLETE
	};

	typedef std::pair<GLenum, FramebufferAttachment> FramebufferAttachmentPair;

	struct Framebuffer {
		GLuint width, height;
		GLuint frambufferID = 0;
		FramebufferStatus status = FramebufferStatus::UNINITIALIZED;
		std::map<GLenum, FramebufferAttachment> attachments;
	};

	Framebuffer* createFramebuffer(unsigned int width, unsigned int height);
	void destroyFramebuffer(Framebuffer* framebuffer);


	bool buildFramebuffer(Framebuffer* framebuffer);

	void bindFramebuffer(const Framebuffer& framebuffer);
	void unbindFramebuffer();

	void clearFramebuffer(const Framebuffer& framebuffer, const Shader& shader);

	void updateFramebufferSize(Framebuffer* framebuffer, unsigned int width, unsigned int height);

	typedef enum class DefaultFramebufferAttachmentType {
		COLOR,
		DEPTH,
		INTEGER
	} DefaultAttachmentType;

	FramebufferAttachmentPair createDefaultFramebufferAttachment(DefaultAttachmentType type, GLuint target = 0);

}
