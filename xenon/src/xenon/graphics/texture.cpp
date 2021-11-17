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

	// Supported formats: JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC
	GLuint loadStandardFormatImage(const std::string& path, TextureDataInfo info, GLenum format) {
		// Load texture data
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);

		GLuint textureID = 0;

		if (info.type == GL_FLOAT) {
			float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
			if (data) {
				textureID = loadImmutableTextureData(data, info, format);
				stbi_image_free(data);
			}
		}
		else {
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
			if (data) {
				textureID = loadImmutableTextureData(data, info, format);
				stbi_image_free(data);
			}
		}

		// Reset state
		stbi_set_flip_vertically_on_load(false);

		return textureID;
	}

	Texture* loadKTXTexture(const std::string& path, GLenum format, const TextureParameters& params) {
		ktxTexture* ktxTex;
		ktx_error_code_e result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &ktxTex);
		if (result != ktx_error_code_e::KTX_SUCCESS) {
			return nullptr;
		}

		// TODO(Neathan): Check if format matches format from ktx metadata
		TextureInfo info = { ktxTex->baseWidth, ktxTex->baseHeight, format };

		GLuint textureID = 0;
		GLenum target, glError;
		ktxTexture_GLUpload(ktxTex, &textureID, &target, &glError);

		ktxTexture_Destroy(ktxTex);

		if (glError != GL_NO_ERROR) {
			return nullptr;
		}

		setTextureParameters(textureID, params);

		return new Texture{
			AssetMetadata(),
			AssetRuntimeData(),
			textureID,
			info,
			params
		};
	}

	Texture* loadTexture(const std::string& path, GLenum format, const TextureParameters& params) {
		// Check if file is supported by stb_image
		int x, y, comp;
		int result = stbi_info(path.c_str(), &x, &y, &comp);
		
		if (result) {
			// Load image using stb_image
			TextureDataInfo dataInfo = { x, y, comp, getDefaultFormat(comp), getTypeFromFormat(format) };
			GLuint textureID = loadStandardFormatImage(path, dataInfo, format);

			setTextureParameters(textureID, params);

			TextureInfo info = { x, y, format };
			return new Texture{
				AssetMetadata(),
				AssetRuntimeData(),
				textureID,
				info,
				params
			};
		}
		else {
			// Try loading image using ktx
			return loadKTXTexture(path, format, params);
		}
	}

	void setTextureParameters(GLuint textureID, const TextureParameters& params) {
		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, params.minFilter);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, params.wrapT);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, params.wrapR);
	}


	Texture* createEmptyTexture(int width, int height, GLenum format, const TextureParameters& params) {
		GLuint textureID;

		glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
		glTextureStorage2D(textureID, 1, format, width, height);

		setTextureParameters(textureID, params);

		TextureInfo info = { width, height, format };
		return new Texture{
			AssetMetadata(),
			AssetRuntimeData(),
			textureID,
			info,
			params
		};
	}

	Texture* createEmptyMultisampledTexture(int width, int height, int samples, GLenum format) {
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &textureID);
		glTextureStorage2DMultisample(textureID, samples, format, width, height, GL_FALSE);

		TextureInfo info = { width, height, format };
		return new Texture{
			AssetMetadata(),
			AssetRuntimeData(),
			textureID,
			info,
			TextureParameters()
		};
	}

	Texture* createEmptyCubemapTexture(int resolution, GLenum format, const TextureParameters& params) {
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);
		glTextureStorage2D(textureID, 1, format, resolution, resolution);

		setTextureParameters(textureID, params);

		TextureInfo info = { resolution, resolution, format };
		return new Texture{
			AssetMetadata(),
			AssetRuntimeData(),
			textureID,
			info,
			params
		};
	}


	//----------------------------------------
	// SECTION: Texture functions
	//----------------------------------------

	GLenum getDefaultFormat(int channels) {
		XE_ASSERT(channels > 0);
		XE_ASSERT(channels <= 4);
		XE_ASSERT(channels != 2);

		if (channels == 1) {
			return GL_RED;
		}
		else if (channels == 3) {
			return GL_RGB;
		}
		return GL_RGBA;
	}

	GLenum getTypeFromFormat(GLenum format) {
		if (isFloatFormat(format)) {
			return GL_FLOAT;
		}
		else if(isUnsignedIntFormat(format)) {
			return GL_UNSIGNED_INT;
		}
		else if (isIntFormat(format)) {
			return GL_INT;
		}
		return GL_UNSIGNED_BYTE;
	}

	bool isFloatFormat(GLenum format) {
		switch (format) {
		case GL_R16F:
		case GL_RG16F:
		case GL_RGB16F:
		case GL_RGBA16F:
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
		case GL_R11F_G11F_B10F:
			return true;
		}
		return false;
	}

	bool isUnsignedIntFormat(GLenum format) {
		switch (format) {
		case GL_RGB10_A2UI:
		case GL_R8UI:
		case GL_R16UI:
		case GL_R32UI:
		case GL_RG8UI:
		case GL_RG16UI:
		case GL_RG32UI:
		case GL_RGB8UI:
		case GL_RGB16UI:
		case GL_RGB32UI:
		case GL_RGBA8UI:
		case GL_RGBA16UI:
		case GL_RGBA32UI:
			return true;
		}
		return false;
	}

	bool isIntFormat(GLenum format) {
		switch (format) {
		case GL_R8I:
		case GL_R16I:
		case GL_R32I:
		case GL_RG8I:
		case GL_RG16I:
		case GL_RG32I:
		case GL_RGB8I:
		case GL_RGB16I:
		case GL_RGB32I:
		case GL_RGBA8I:
		case GL_RGBA16I:
		case GL_RGBA32I:
			return true;
		}
		return false;
	}

	bool isSRGBFormat(GLenum format) {
		switch (format) {
		case GL_SRGB8:
		case GL_SRGB8_ALPHA8:
			return true;
		default:
			return false;
		}
	}

}
