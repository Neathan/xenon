#include "texture.h"

#include <map>

#include <stb_image.h>
#include <ktx.h>

#include "xenon/core/assert.h"
#include "xenon/core/log.h"

namespace xe {

	//----------------------------------------
	// SECTION: Texture
	//----------------------------------------

	template<typename T>
	Texture* loadTextureInternal(const T* data, int width, int height, int channels, TextureFormat format, const TextureParameters& params) {
		// Create GL texture
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, params.minFilter);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, params.wrapT);

		glTextureStorage2D(textureID, 1, getTextureFormatInternalFormat(format), width, height);
		glTextureSubImage2D(textureID, 0, 0, 0, width, height, getTextureFormatBaseFormat(format), getTextureFormatDataType(format), data);

		return new Texture{ AssetMetadata(), AssetRuntimeData(), textureID, params, width, height, channels, format };
	}

	Texture* loadTexture(const std::string& path, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{}) {
		// Load texture data
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);

		Texture* texture;
		if (!isTextureFormatFloatFormat(format)) {
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
			if (!data) {
				return nullptr;
			}
			texture = loadTextureInternal(data, width, height, channels, format, params);
			stbi_image_free(data);
		}
		else {
			float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
			if (!data) {
				return nullptr;
			}
			texture = loadTextureInternal(data, width, height, channels, format, params);
			stbi_image_free(data);
		}
		stbi_set_flip_vertically_on_load(false);

		return texture;
	}

	Texture* loadTexture(const unsigned char* data, int width, int height, int channels, TextureFormat format = TextureFormat::RGBA, const TextureParameters& params = TextureParameters{}) {
		return loadTextureInternal(data, width, height, channels, format, params);
	}

	Texture* loadKTXTexture(const std::string& path, const TextureParameters& params) {
		Texture* texture;

		ktxTexture* ktxTexture;
		ktx_error_code_e result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &ktxTexture);
		if (result != ktx_error_code_e::KTX_SUCCESS) {
			return nullptr;
		}

		GLuint textureID = 0;
		GLenum target, glError;
		ktxTexture_GLUpload(ktxTexture, &textureID, &target, &glError);

		if (glError != GL_NO_ERROR) {
			return nullptr;
		}

		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, params.minFilter);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, params.wrapT);

		// TODO: Extract texture metadata and include missing info in texture
		texture = new Texture{ AssetMetadata(), AssetRuntimeData(), textureID, TextureParameters(), (int)ktxTexture->baseWidth, (int)ktxTexture->baseHeight, 0, TextureFormat::UNKNOWN };

		ktxTexture_Destroy(ktxTexture);

		return texture;
	}

	Texture* createEmptyTexture(int width, int height, TextureFormat format, const TextureParameters& params, int samples) {
		XE_ASSERT(samples >= 1);

		GLuint textureID;
		if (samples == 1) {
			glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
			glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, params.minFilter);
			glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, params.magFilter);
			glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, params.wrapS);
			glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, params.wrapT);
		}
		else {
			glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &textureID);
		}


		if (samples == 1) {
			glTextureStorage2D(textureID, 1, getTextureFormatInternalFormat(format), width, height);
		}
		else {
			glTextureStorage2DMultisample(textureID, samples, getTextureFormatInternalFormat(format), width, height, GL_FALSE);
		}

		return new Texture{ AssetMetadata(), AssetRuntimeData(), textureID, params, width, height, 0, format };
	}

	Texture* createEmptyCubemapTexture(int resolution, TextureFormat format, const TextureParameters& params) {
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);

		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, params.minFilter);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, params.wrapT);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, params.wrapR);

		glTextureStorage2D(textureID, 1, getTextureFormatInternalFormat(format), resolution, resolution);
		/*for (int face = 0; face < 6; ++face) {
			glTextureSubImage3D(textureID, 0, 0, 0, face, resolution, resolution, 1, getTextureFormatBaseFormat(format), getTextureFormatDataType(format), nullptr);
		}*/

		return new Texture{ AssetMetadata(), AssetRuntimeData(), textureID, params, resolution, resolution, 0, format };
	}


	//----------------------------------------
	// SECTION: Texture functions
	//----------------------------------------

	// TODO: Replace these helper functions with static lookup data
	TextureFormat channelsToFormat(int channels, bool sRGB) {
		XE_ASSERT(channels > 0);
		XE_ASSERT(channels <= 4);
		XE_ASSERT(channels != 2);

		if (channels == 1) {
			return TextureFormat::RED;
		}
		else if (channels == 3) {
			return sRGB ? TextureFormat::SRGB : TextureFormat::RGB;
		}
		return sRGB ? TextureFormat::SRGBA : TextureFormat::RGBA;
	}

	GLenum getTextureFormatInternalFormat(TextureFormat format) {
		if (format == TextureFormat::SRGB) {
			return GL_SRGB8;
		}
		else if (format == TextureFormat::SRGBA) {
			return GL_SRGB8_ALPHA8;
		}
		else if (format == TextureFormat::RGB) {
			return GL_RGB8;
		}
		else if (format == TextureFormat::RGBA) {
			return GL_RGBA8;
		}
		else if (format == TextureFormat::RED) {
			return GL_R32UI;
		}
		else if (format == TextureFormat::RGB_FLOAT) {
			return GL_RGB16F;
		}
		else if (format == TextureFormat::RGBA_FLOAT) {
			return GL_RGBA16F;
		}
		else if (format == TextureFormat::DEPTH) {
			return GL_DEPTH_COMPONENT24;
		}
		return GL_INVALID_ENUM;
	}

	GLenum getTextureFormatBaseFormat(TextureFormat format) {
		if (format == TextureFormat::RGB || format == TextureFormat::SRGB || format == TextureFormat::RGB_FLOAT) {
			return GL_RGB;
		}
		else if (format == TextureFormat::RGBA || format == TextureFormat::SRGBA || format == TextureFormat::RGBA_FLOAT) {
			return GL_RGBA;
		}
		else if (format == TextureFormat::RED) {
			return GL_RED;
		}
		else if (format == TextureFormat::DEPTH) {
			return GL_DEPTH_COMPONENT;
		}
		return GL_INVALID_ENUM;
	}

	GLenum getTextureFormatDataType(TextureFormat format) {
		if (format == TextureFormat::RED) {
			return GL_UNSIGNED_INT;
		}
		else if (format == TextureFormat::RGB_FLOAT || format == TextureFormat::RGBA_FLOAT || format == TextureFormat::DEPTH) {
			return GL_FLOAT;
		}
		return GL_UNSIGNED_BYTE;
	}

	bool isTextureFormatFloatFormat(TextureFormat format) {
		if (format == TextureFormat::RGB_FLOAT || format == TextureFormat::RGBA_FLOAT || format == TextureFormat::DEPTH) {
			return true;
		}
		return false;
	}

	//----------------------------------------
	// SECTION: Texture asset functions
	//----------------------------------------

	Texture* createTextureAsset(AssetManager* manager, const std::string& path, const unsigned char* data, int width, int height, int channels, TextureFormat format, const TextureParameters& params) {
		Texture* texture = loadTexture(data, width, height, channels, format, params);
		Asset* asset = createAsset(manager, path, AssetType::Texture, UUID::None());
		copyAssetMetaRuntimeData(asset, texture);
		return texture;
	}

	Texture* createInternalTextureAsset(AssetManager* manager, const std::string& hostPath, const std::string& internalPath, const unsigned char* data, int width, int height, int channels, TextureFormat format, const TextureParameters& params) {
		return createTextureAsset(manager, XE_HOST_PATH_BEGIN + hostPath + XE_HOST_PATH_END + internalPath, data, width, height, channels, format, params);
	}


	void TextureSerializer::serialize(Asset* asset) const {
		// TODO: Implement
		XE_LOG_ERROR("Texture serialization is not yet implemented.");
	}

	bool TextureSerializer::loadData(Asset** asset) const {
		const Asset* sourceAsset = *asset;
		*asset = loadTexture((*asset)->metadata.path);
		copyAssetMetaRuntimeData(sourceAsset, *asset);
		return true;
	}

}
