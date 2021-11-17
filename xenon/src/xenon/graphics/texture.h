#pragma once

#include <string>

#include <glad/gl.h>

#include "xenon/core/asset.h"
#include "xenon/core/asset_manager.h"

namespace xe {
	
	//----------------------------------------
	// SECTION: Texture
	//----------------------------------------

	struct TextureDataInfo {
		int width;
		int height;
		int channels;
		GLenum format;
		GLenum type;
	};

	struct TextureInfo {
		int width;
		int height;
		GLenum format;
	};

	struct TextureParameters {
		GLenum minFilter = GL_LINEAR;
		GLenum magFilter = GL_LINEAR;
		GLenum wrapS = GL_REPEAT;
		GLenum wrapT = GL_REPEAT;
		GLenum wrapR = GL_REPEAT;
	};

	struct Texture : Asset {
		const GLuint textureID;
		const TextureInfo info;
		const TextureParameters params;
	};

	Texture* loadTexture(const std::string& path, GLenum format = GL_RGBA8, const TextureParameters& params = {});
	
	template<typename T>
	GLuint loadImmutableTextureData(const T* data, TextureDataInfo info, GLenum format) {
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

		glTextureStorage2D(textureID, 1, format, info.width, info.height);
		glTextureSubImage2D(textureID, 0, 0, 0, info.width, info.height, info.format, info.type, data);

		return textureID;
	}

	void setTextureParameters(GLuint textureID, const TextureParameters& params);
	
	Texture* createEmptyTexture(int width, int height, GLenum format = GL_RGBA8, const TextureParameters& params = {});
	Texture* createEmptyMultisampledTexture(int width, int height, int samples, GLenum format = GL_RGBA8);
	Texture* createEmptyCubemapTexture(int resolution, GLenum format = GL_RGBA8, const TextureParameters& params = {});
	

	//----------------------------------------
	// SECTION: Texture functions
	//----------------------------------------

	GLenum getDefaultFormat(int channels);
	GLenum getTypeFromFormat(GLenum format);
	bool isFloatFormat(GLenum format);
	bool isUnsignedIntFormat(GLenum format);
	bool isIntFormat(GLenum format);
	bool isSRGBFormat(GLenum format);

}
