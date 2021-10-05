#include "asset.h"

#include "xenon/core/log.h"

namespace xe {

	void copyAssetMetaRuntimeData(const Asset* source, Asset* target) {
		target->metadata.handle = source->metadata.handle;
		target->metadata.type = source->metadata.type;
		target->metadata.path = source->metadata.path;
		target->runtimeData.loaded = source->runtimeData.loaded;
		target->runtimeData.parent = source->runtimeData.parent;
		target->runtimeData.filename = source->runtimeData.filename;
		target->runtimeData.extension = source->runtimeData.extension;
	}

	AssetType getAssetTypeFromPath(const std::string& path) {
		auto index = path.find_last_of('.');
		if (index != std::string::npos) {
			std::string extension = path.substr(index + 1);

			// Images
			if (extension == "png") return AssetType::Texture;
			if (extension == "jpg") return AssetType::Texture;
			if (extension == "jpeg") return AssetType::Texture;
			if (extension == "tga") return AssetType::Texture;
			if (extension == "bmp") return AssetType::Texture;
			if (extension == "psd") return AssetType::Texture;
			if (extension == "ktx") return AssetType::Texture;

			// Models
			if (extension == "glb") return AssetType::Model;
			if (extension == "gltf") return AssetType::Model;
		}

		XE_LOG_TRACE_F("ASSET: No extension match for: {}", path);
		return AssetType::None;
	}

}
