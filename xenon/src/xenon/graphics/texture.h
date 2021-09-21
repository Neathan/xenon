#pragma once

#include <string>
#include <memory>

#include <glad/gl.h>

namespace xe {
	
	//----------------------------------------
	// SECTION: Texture
	//----------------------------------------

	#define TEXTURE_INTERNAL_SIGNATURE "?i-"

	// Note: These should match shader samplers
	enum class TextureType : unsigned int {
		ALBEDO = 0,
		METALLIC_ROUGHNESS = 1,
		NORMAL = 2,
		AO = 3,
		EMISSIVE = 4,
		IBL_CUBEMAP = 5,

		IBL_SOURCE = 10,

		GENERIC_RGB = 20,
		GENERIC_RGBA = 21,
		GENERIC_FLOAT = 22,
		GENERIC_RED = 23,
	};

	enum class TextureFormat : GLenum {
		RED = 0,
		RGB = 1,
		RGBA = 2,

		RGB_FLOAT = 3,
		RGBA_FLOAT = 4,

		SRGB = 5,
		SRGBA = 6,

		DEPTH = 7
	};

	struct TextureParameters {
		GLenum minFilter = GL_LINEAR;
		GLenum magFilter = GL_LINEAR;
		GLenum wrapS = GL_REPEAT;
		GLenum wrapT = GL_REPEAT;
		GLenum wrapR = GL_REPEAT;
	};

	struct Texture {
		const GLuint textureID;
		const TextureType type;
		const TextureParameters params;

		const int width;
		const int height;
		const int channels;
		const TextureFormat format;

		const std::string sourcePath;

		~Texture();
	};

	std::shared_ptr<Texture> loadTexture(const std::string& path, TextureType type, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{});
	std::shared_ptr<Texture> loadTexture(const std::string& signature, const unsigned char* data, int width, int height, int channels, TextureType type, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{});
	// TODO: Add signature float data function

	std::shared_ptr<Texture> createEmptyTexture(int width, int height, TextureType type, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{});
	std::shared_ptr<Texture> createEmptyCubemapTexture(int resolution, TextureType cubemapType, TextureFormat format, const TextureParameters& params = TextureParameters{});


	//----------------------------------------
	// SECTION: Texture functions
	//----------------------------------------

	TextureFormat channelsToFormat(int channels, bool sRGB = false);
	GLenum getTextureFormatInternalFormat(TextureFormat format);
	GLenum getTextureFormatBaseFormat(TextureFormat format);
	GLenum getTextureFormatDataType(TextureFormat format);
	bool isTextureFormatFloatFormat(TextureFormat format);

	void clearUnusedTextures();
	void clearTextureCache();

}
