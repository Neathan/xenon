#include "texture.h"

#include <map>

#include <stb_image.h>

#include "xenon/core/assert.h"
#include "xenon/core/log.h"

namespace xe {

	//----------------------------------------
	// SECTION: Texture
	//----------------------------------------

	static std::map<std::string, std::weak_ptr<Texture>> s_textureCache;

	Texture::~Texture() {
		// TODO: Fix de-construction issue
		//glDeleteTextures(1, &textureID);
	}

	template<typename T>
	std::shared_ptr<Texture> loadTextureInternal(const std::string& signature, const T* data, int width, int height, int channels, TextureType type, TextureFormat format, const TextureParameters& params) {
		// Create GL texture
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, params.minFilter);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, params.wrapT);

		glTextureStorage2D(textureID, 1, getTextureFormatInternalFormat(format), width, height);
		glTextureSubImage2D(textureID, 0, 0, 0, width, height, getTextureFormatBaseFormat(format), getTextureFormatDataType(format), data);

		// Create and add to cache
		std::shared_ptr<Texture> texture = std::make_shared<Texture>(Texture{ textureID, type, params, width, height, channels, format, signature });
		s_textureCache.emplace(signature, texture);

		return texture;
	}

	std::shared_ptr<Texture> checkCache(const std::string& signature) {
		if (s_textureCache.find(signature) != s_textureCache.end()) {
			std::weak_ptr<Texture> cacheHit = s_textureCache.at(signature);
			if (cacheHit.expired()) {
				s_textureCache.erase(signature);
			}
			else {
				XE_LOG_TRACE_F("TEXTURE CACHE: Found texture signature in cache: {}", signature);
				return cacheHit.lock();
			}
		}
		return nullptr;
	}

	std::shared_ptr<Texture> loadTexture(const std::string& path, TextureType type, TextureFormat format, const TextureParameters& params) {
		// Check cache for texture
		auto cache = checkCache(path);
		if (cache) {
			return cache;
		}
		
		// Load texture data
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);

		std::shared_ptr<Texture> texture;
		if (!isTextureFormatFloatFormat(format)) {
			unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
			texture = loadTextureInternal(path, data, width, height, channels, type, format, params);
			stbi_image_free(data);
		}
		else {
			float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
			texture = loadTextureInternal(path, data, width, height, channels, type, format, params);
			stbi_image_free(data);
		}

		return texture;
	}

	std::shared_ptr<Texture> loadTexture(const std::string& signature, const unsigned char* data, int width, int height, int channels, TextureType type, TextureFormat format, const TextureParameters& params) {
		// Check cache for texture
		auto cache = checkCache(signature);
		if (cache) {
			return cache;
		}

		return loadTextureInternal(signature, data, width, height, channels, type, format, params);
	}

	std::shared_ptr<Texture> createEmptyTexture(int width, int height, TextureType type, TextureFormat format, const TextureParameters& params) {
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, params.minFilter);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, params.wrapT);

		glTextureStorage2D(textureID, 1, getTextureFormatInternalFormat(format), width, height);

		return std::make_shared<Texture>(Texture{ textureID, type, params, width, height, 0, format, "uncached-texture" });;
	}

	std::shared_ptr<Texture> createEmptyCubemapTexture(int resolution, TextureType cubemapType, TextureFormat format, const TextureParameters& params) {
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

		return std::make_shared<Texture>(Texture{ textureID, cubemapType, params, resolution, resolution, 0, format, "cubemap-no-signature" });
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


	void clearUnusedTextures() {
		for (auto it = s_textureCache.cbegin(); it != s_textureCache.cend(); ) {
			if (it->second.expired()) {
				it = s_textureCache.erase(it);
				continue;
			}
			++it;
		}
	}

	void clearTextureCache() {
		s_textureCache.clear();
	}

}
