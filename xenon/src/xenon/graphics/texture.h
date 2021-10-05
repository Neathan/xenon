#pragma once

#include <string>

#include <glad/gl.h>

#include "xenon/core/asset.h"
#include "xenon/core/asset_manager.h"

namespace xe {
	
	//----------------------------------------
	// SECTION: Texture
	//----------------------------------------

	/*
		ALBEDO = 0,
		METALLIC_ROUGHNESS = 1,
		NORMAL = 2,
		AO = 3,
		EMISSIVE = 4,
		IRRADIANCE_CUBEMAP = 5,
		RADIANCE_CUBEMAP = 6,
		BRDF_LUT = 7,

		IBL_SOURCE = 10,
	*/

	/*enum class TextureType : unsigned int {
		RGB,
		RGBA,
		FLOAT,
		RED,

		LAST = RED
	};*/

	enum class TextureFormat : GLenum {
		RED = 0,
		RGB = 1,
		RGBA = 2,

		RGB_FLOAT = 3,
		RGBA_FLOAT = 4,

		SRGB = 5,
		SRGBA = 6,

		DEPTH = 7,

		UNKNOWN = -1
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
		const TextureParameters params;

		const int width;
		const int height;
		const int channels;
		const TextureFormat format;
	};

	/*
	Texture* loadTexture(const std::string& path, TextureType type = TextureType::GENERIC_RGBA, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{});
	Texture* loadTexture(const unsigned char* data, int width, int height, int channels, TextureType type, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{});
	// TODO: Add signature float data function
	*/

	// TODO: Make private
	Texture* loadKTXTexture(const std::string& path, const TextureParameters& params = TextureParameters{});
	

	Texture* createEmptyTexture(int width, int height, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{}, int samples = 1);
	Texture* createEmptyCubemapTexture(int resolution, TextureFormat format, const TextureParameters& params = TextureParameters{});
	

	//----------------------------------------
	// SECTION: Texture functions
	//----------------------------------------

	TextureFormat channelsToFormat(int channels, bool sRGB = false);
	GLenum getTextureFormatInternalFormat(TextureFormat format);
	GLenum getTextureFormatBaseFormat(TextureFormat format);
	GLenum getTextureFormatDataType(TextureFormat format);
	bool isTextureFormatFloatFormat(TextureFormat format);

	//----------------------------------------
	// SECTION: Texture asset functions
	//----------------------------------------

	Texture* createTextureAsset(AssetManager* manager, const std::string& path, const unsigned char* data, int width, int height, int channels, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{});
	Texture* createInternalTextureAsset(AssetManager* manager, const std::string& hostPath, const std::string& internalPath, const unsigned char* data, int width, int height, int channels, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{});

	struct TextureSerializer : AssetSerializer {
		void serialize(Asset* asset) const override;
		bool loadData(Asset** asset) const override;
	};
}
