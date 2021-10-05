#pragma once

#include <glm/glm.hpp>

#include <string>
#include <memory>

#include "xenon/graphics/texture.h"

#include "xenon/core/asset.h"

namespace xe {

	struct PBRMetallicRoughness {
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		Texture* baseColorTexture = nullptr;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		Texture* metallicRoughnessTexture = nullptr;
		// extension
		// extras
	};

	// TODO: Find a way to use the OPAQUE keyword (currently defined by wingdi.h)
	enum class AlphaMode : int {
		OPAQUE_IGNORE = 0,
		MASK = 1,
		BLEND = 2
	};

	struct Material : Asset {
		std::string name;
		// extensions
		// extras
		PBRMetallicRoughness pbrMetallicRoughness;
		Texture* normalTexture = nullptr;
		Texture* occlusionTexture = nullptr;
		Texture* emissiveTexture = nullptr;
		glm::vec3 emissiveFactor = glm::vec3(0.0f);
		AlphaMode alphaMode = AlphaMode::OPAQUE_IGNORE;
		float alphaCutoff = 0.5f;
		bool doubleSided = false;
	};

}
