#include "framebuffer.h"

#include "xenon/core/log.h"
#include "xenon/core/assert.h"

namespace xe {

	Framebuffer* createFramebuffer(unsigned int width, unsigned int height, int samples) {
		Framebuffer* framebuffer = new Framebuffer{ width, height, samples };
		glCreateFramebuffers(1, &framebuffer->frambufferID);
		return framebuffer;
	}

	void destroyFramebufferInternals(Framebuffer* framebuffer) {
		glDeleteFramebuffers(1, &framebuffer->frambufferID);
		framebuffer->frambufferID = 0;

		for (auto& [target, attachment] : framebuffer->attachments) {
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

		// Check for valid size
		if (framebuffer->width == 0 || framebuffer->height == 0) {
			XE_LOG_WARN("FRAMEBUFFER: Invalid width or height");
			return false;
		}

		if (framebuffer->status != FramebufferStatus::UNINITIALIZED) {
			// TODO: Destroy previous textures
			// TODO: Manage incomplete state
			framebuffer->colorBuffers.clear();
		}

		// Create attachments
		for (auto& [target, attachment] : framebuffer->attachments) {
			if (attachment.texture) {
				XE_LOG_WARN_F("FRAMEBUFFER: Framebuffer texture already exists, overwriting framebuffer texture: {}", attachment.texture->textureID);
			}
			// Create appropriate texture for sample count
			if (framebuffer->samples == 1) {
				attachment.texture = createEmptyTexture(framebuffer->width, framebuffer->height, attachment.format, attachment.textureParams);
			}
			else {
				attachment.texture = createEmptyMultisampledTexture(framebuffer->width, framebuffer->height, framebuffer->samples, attachment.format);
			}
			glNamedFramebufferTexture(framebuffer->frambufferID, attachment.target, attachment.texture->textureID, 0);

			if (attachment.target != GL_DEPTH_ATTACHMENT && attachment.target != GL_STENCIL_ATTACHMENT) {
				framebuffer->colorBuffers.push_back(attachment.target);
			}
		}
		if (framebuffer->colorBuffers.size()) {
			glNamedFramebufferDrawBuffers(framebuffer->frambufferID, framebuffer->colorBuffers.size(), framebuffer->colorBuffers.data());
		}
		else {
			XE_LOG_DEBUG_F("FRAMEBUFFER: Framebuffer has no drawbuffers (depth only)");
		}

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
		
		/*static GLfloat zeroF = 0;
		static GLint zeroI = 0;
		glClearNamedFramebufferfv(framebuffer.frambufferID, GL_COLOR, 0, &zeroF);
		glClearNamedFramebufferiv(framebuffer.frambufferID, GL_COLOR, 1, &zeroI);
		glClearNamedFramebufferfv(framebuffer.frambufferID, GL_DEPTH, 0, &zeroF);*/
		
		unbindShader();
	}

	void updateFramebufferSize(Framebuffer* framebuffer, unsigned int width, unsigned int height) {
		destroyFramebufferInternals(framebuffer);
		framebuffer->width = width;
		framebuffer->height = height;
		glCreateFramebuffers(1, &framebuffer->frambufferID);
		buildFramebuffer(framebuffer);
	}

	void blitFramebuffers(Framebuffer* source, Framebuffer* target) {
		for (const auto [attachmentTarget, attachment] : source->attachments) {
			if (target->attachments.find(attachmentTarget) != target->attachments.end()) {
				glNamedFramebufferReadBuffer(source->frambufferID, attachmentTarget);
				glNamedFramebufferDrawBuffer(target->frambufferID, attachmentTarget);
				GLbitfield filter = GL_COLOR_BUFFER_BIT;
				if (attachmentTarget == GL_DEPTH_ATTACHMENT) {
					filter = GL_DEPTH_ATTACHMENT;
				}
				else if(attachmentTarget == GL_STENCIL_ATTACHMENT) {
					filter = GL_STENCIL_BUFFER_BIT;
				}
				glBlitNamedFramebuffer(source->frambufferID, target->frambufferID, 0, 0, source->width, source->height, 0, 0, target->width, target->height, filter, GL_NEAREST);
			}
		}
		// Restore draw and read buffers
		if (source->colorBuffers.size() > 0) {
			glNamedFramebufferReadBuffer(source->frambufferID, source->colorBuffers.at(0));
		}
		if (target->colorBuffers.size() > 0) {
			glNamedFramebufferDrawBuffers(target->frambufferID, target->colorBuffers.size(), target->colorBuffers.data());
		}
	}


	FramebufferAttachmentPair createDefaultFramebufferAttachment(DefaultAttachmentType type, GLuint target) {
		if (type == DefaultAttachmentType::COLOR) {
			return { GL_COLOR_ATTACHMENT0 + target, FramebufferAttachment{ GL_COLOR_ATTACHMENT0 + target, GL_RGB8, TextureParameters{ GL_LINEAR, GL_LINEAR } } };
		}
		else if (type == DefaultAttachmentType::DEPTH) {
			XE_ASSERT(target == 0);
			return { GL_DEPTH_ATTACHMENT, FramebufferAttachment{ GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24, TextureParameters{ GL_NEAREST, GL_NEAREST } } };
		}
		else { // type == DefaultAttachmentType::INTEGER
			return { GL_COLOR_ATTACHMENT0 + target, FramebufferAttachment{ GL_COLOR_ATTACHMENT0 + target, GL_R32UI, TextureParameters{ GL_NEAREST, GL_NEAREST } } };
		}
	}

}
