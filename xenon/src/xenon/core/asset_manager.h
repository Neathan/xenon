#pragma once

#include <map>
#include <unordered_map>
#include <vector>

#include "xenon/core/asset.h"
#include "xenon/core/log.h"

#define XE_HOST_PATH_BEGIN "*"
#define XE_HOST_PATH_END "?"


namespace xe {

	struct AssetManager {
		std::string projectFolder;
		std::map<UUID, Asset*> assets;
		std::map<std::string, AssetMetadata> registry;
		std::map<AssetType, AssetSerializer*> serializers;

		std::vector<std::pair<UUID, Asset*>> sortedAssets;
	};

	AssetManager* createAssetManager(const std::string& projectFolder);
	void destroyAssetManager(AssetManager* manager);

	// TODO: Can this be avoided?
	AssetManager* getAssetManager();

	bool loadAssetData(AssetManager* manager, Asset** asset);
	Asset* createAsset(AssetManager* manager, const std::string& path, AssetType type, UUID parent);
	void importAsset(AssetManager* manager, const std::string& path, UUID parent);

	UUID updateDirectoryAssets(AssetManager* manager, const std::string& path, UUID parent);
	void updateAssetRegistry(AssetManager* manager);

	template<typename T>
	T* getAsset(AssetManager* manager, UUID assetHandle, bool loadData = true) {
		if (manager->assets.find(assetHandle) == manager->assets.end()) {
			XE_LOG_DEBUG_F("ASSET_MANAGER: Asset not found: {}", assetHandle);
			return nullptr;
		}
		Asset* asset = manager->assets[assetHandle];

		if (loadData && !asset->runtimeData.loaded) {
			if (!loadAssetData(manager, &asset)) {
				XE_LOG_ERROR_F("ASSET_MANAGER: Failed to load data for asset: {}", assetHandle);
				return nullptr;
			}
		}
		return static_cast<T*>(asset);
	}

	template<typename T>
	T* getAsset(AssetManager* manager, const std::string& path, bool loadData = true) {
		for (const auto& [id, asset] : manager->assets) {
			if (asset->metadata.path == path) {
				return getAsset<T>(manager, id, loadData);
			}
		}
		XE_LOG_ERROR_F("ASSET_MANAGER: Asset not found: {}", path);
		return nullptr;
	}


}
