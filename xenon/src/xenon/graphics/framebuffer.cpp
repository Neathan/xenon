#include "framebuffer.h"

#include "xenon/core/log.h"
#include "xenon/core/assert.h"

namespace xe {

	Framebuffer* createFramebuffer(unsigned int width, unsigned int height) {
		Framebuffer* framebuffer = new Framebuffer{ width, height };
		glCreateFramebuffers(1, &framebuffer->frambufferID);
		return framebuffer;
	}

	void destroyFramebufferInternals(Framebuffer* framebuffer) {
		glDeleteFramebuffers(1, &framebuffer->frambufferID);
		framebuffer->frambufferID = 0;

		for (auto [target, attachment] : framebuffer->attachments) {
			if (attachment.texture) {
				glDeleteTextures(1, &attachment.texture->textureID);
				attachment.texture = nullptr;
			}
		}
	}

	void destroyFramebuffer(Framebuffer* framebuffer) {
		destroyFramebufferInternals(framebuffer);
		delete framebuffer;
	}

	bool buildFramebuffer(Framebuffer* framebuffer) {
		XE_ASSERT(framebuffer->frambufferID != 0);

		if (framebuffer->width == 0 || framebuffer->height == 0) {
			XE_LOG_WARN("FRAMEBUFFER: Invalid width or height");
			return false;
		}

		if (framebuffer->status != FramebufferStatus::UNINITIALIZED) {
			// TODO: Destroy previous textures
			// TODO: Manage incomplete state
		}

		std::vector<GLenum> drawBuffers;

		for (auto& [target, attachment] : framebuffer->attachments) {
			if (attachment.texture) {
				XE_LOG_WARN_F("Framebuffer texture already exists, overwriting framebuffer texture: {}", attachment.texture->sourcePath);
			}
			attachment.texture = createEmptyTexture(framebuffer->width, framebuffer->height, attachment.type, attachment.format, attachment.textureParams);
			glNamedFramebufferTexture(framebuffer->frambufferID, attachment.target, attachment.texture->textureID, 0);

			if (attachment.target != GL_DEPTH_ATTACHMENT) {
				drawBuffers.push_back(attachment.target);
			}
		}
		glNamedFramebufferDrawBuffers(framebuffer->frambufferID, drawBuffers.size(), drawBuffers.data());

		GLenum status = glCheckNamedFramebufferStatus(framebuffer->frambufferID, GL_FRAMEBUFFER);
		if (status == GL_FRAMEBUFFER_COMPLETE) {
			framebuffer->status = FramebufferStatus::COMPLETE;
		}
		else {
			XE_LOG_ERROR("FRAMEBUFFER: Framebuffer is incomplete");
			framebuffer->status = FramebufferStatus::INCOMPLETE;
			return false;
		}

		return true;
	}

	void bindFramebuffer(const Framebuffer& framebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.frambufferID);
	}

	void unbindFramebuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void clearFramebuffer(const Framebuffer& framebuffer, const Shader& shader) {
		bindShader(shader);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		/*
		static GLfloat zeroF = 0;
		static GLint zeroI = 0;
		glClearNamedFramebufferfv(framebuffer->frambufferID, GL_COLOR, 0, &zeroF);
		glClearNamedFramebufferiv(framebuffer->frambufferID, GL_COLOR, 1, &zeroI);
		glClearNamedFramebufferfv(framebuffer->frambufferID, GL_DEPTH, 0, &zeroF);
		*/
		unbindShader();
	}

	void updateFramebufferSize(Framebuffer* framebuffer, unsigned int width, unsigned int height) {
		destroyFramebufferInternals(framebuffer);
		framebuffer->width = width;
		framebuffer->height = height;
		glCreateFramebuffers(1, &framebuffer->frambufferID);
		buildFramebuffer(framebuffer);
	}


	FramebufferAttachmentPair createDefaultFramebufferAttachment(DefaultAttachmentType type, GLuint target) {
		if (type == DefaultAttachmentType::COLOR) {
			return { GL_COLOR_ATTACHMENT0 + target, FramebufferAttachment{ GL_COLOR_ATTACHMENT0 + target, TextureType::GENERIC_RGB, TextureFormat::RGB, TextureParameters{ GL_LINEAR, GL_LINEAR } } };
		}
		else if (type == DefaultAttachmentType::DEPTH) {
			XE_ASSERT(target == 0);
			return { GL_DEPTH_ATTACHMENT, FramebufferAttachment{ GL_DEPTH_ATTACHMENT, TextureType::GENERIC_FLOAT, TextureFormat::DEPTH, TextureParameters{ GL_NEAREST, GL_NEAREST } } };
		}
		else { // type == DefaultAttachmentType::INTEGER
			return { GL_COLOR_ATTACHMENT0 + target, FramebufferAttachment{ GL_COLOR_ATTACHMENT0 + target, TextureType::GENERIC_RED, TextureFormat::RED, TextureParameters{ GL_NEAREST, GL_NEAREST } } };
		}
	}

}
