#pragma once

#include <string>
#include <vector>

#include "xenon/core/uuid.h"

namespace xe {

	enum class AssetType {
		None,
		Texture,
		Model,

		Directory,
	};

	struct AssetMetadata {
		UUID handle = UUID::None();
		AssetType type = AssetType::None;
		std::string path;
	};

	struct AssetRuntimeData {
		bool loaded = false;
		bool updated = false;
		UUID parent = UUID::None();

		std::string filename;
		std::string extension;
	};

	struct Asset {
		AssetMetadata metadata;
		AssetRuntimeData runtimeData;

		/*
		virtual bool operator==(const Asset& other) const {
			return metadata.handle == other.metadata.handle;
		}
		virtual bool operator!=(const Asset& other) const {
			return metadata.handle != other.metadata.handle;
		}
		*/
	};
	
	void copyAssetMetaRuntimeData(const Asset* source, Asset* target);
	AssetType getAssetTypeFromPath(const std::string& path);

	struct Directory : Asset {
		std::vector<UUID> children;
	};

	struct AssetSerializer {
		virtual void serialize(Asset* asset) const = 0;
		virtual bool loadData(Asset** asset) const = 0;
	};


}
