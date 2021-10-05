#include "asset_manager.h"

#include <filesystem>

#include "xenon/graphics/texture.h"
#include "xenon/graphics/model.h"

namespace xe {

	static AssetManager* s_assetManager = nullptr;

	AssetManager* createAssetManager(const std::string& projectFolder) {
		AssetManager* manager = new AssetManager();
		manager->projectFolder = projectFolder;

		// Load serializers
		manager->serializers.emplace(AssetType::Model, new ModelSerializer());
		manager->serializers.emplace(AssetType::Texture, new TextureSerializer());

		// Load project assets
		updateDirectoryAssets(manager, projectFolder, UUID::None());
		updateAssetRegistry(manager);

		s_assetManager = manager;

		return manager;
	}

	void destroyAssetManager(AssetManager* manager) {
		for (auto [type, serializer] : manager->serializers) {
			delete serializer;
		}
		// TODO: Destroy asset
		delete manager;
	}

	AssetManager* getAssetManager() {
		return s_assetManager;
	}

	bool loadAssetData(AssetManager* manager, Asset** asset) {
		if ((*asset)->metadata.type == AssetType::Directory) {
			return false;
		}

		(*asset)->runtimeData.loaded = manager->serializers.at((*asset)->metadata.type)->loadData(asset);
		return (*asset)->runtimeData.loaded;
	}

	Asset* createAsset(AssetManager* manager, const std::string& path, AssetType type, UUID parent) {
		Asset* asset = nullptr;

		if (type == AssetType::Directory) {
			asset = new Directory();
		}
		else {
			asset = new Asset();
		}

		auto filenameIndex = path.find_last_of('/');
		auto extentionIndex = path.find_last_of('.');

		asset->metadata.path = path;
		// No filename separator
		if (filenameIndex == std::string::npos) {
			asset->runtimeData.filename = "";
		}
		// Nothing after separator
		else if (filenameIndex + 1 == path.length()) {
			asset->runtimeData.filename = path.substr(0, filenameIndex);
		}
		else {
			asset->runtimeData.filename = path.substr(filenameIndex + 1);
		}
		asset->runtimeData.extension = extentionIndex != std::string::npos ? path.substr(extentionIndex + 1) : "";

		// Check if asset is already in registry
		if (manager->registry.find(asset->metadata.path) != manager->registry.end()) {
			// Copy metadata
			AssetMetadata& metadata = manager->registry[asset->metadata.path];
			asset->metadata.handle = metadata.handle;
			asset->metadata.type = metadata.type;

			// Check for miss matching types
			if (asset->metadata.type != type) {
				// TODO: Manage properly, should be treated as new asset and previous data should be cleared
				XE_LOG_ERROR_F("ASSET_MANAGER: Asset type missmatch: {}", asset->metadata.path);
				asset->metadata.type = AssetType::None;
			}
		}
		else {
			// If new asset
			asset->metadata.handle = UUID();
			asset->metadata.type = type;
		}

		asset->runtimeData.parent = parent;
		asset->runtimeData.loaded = false;
		return asset;
	}

	void importAsset(AssetManager* manager, const std::string& path, UUID parent) {
		AssetType type = getAssetTypeFromPath(path);
		Asset* asset = createAsset(manager, path, type, parent);

		// Register asset if needed
		if (manager->registry.find(asset->metadata.path) == manager->registry.end()) {
			manager->registry.emplace(asset->metadata.path, asset->metadata);
		}

		// Add/replace asset
		if (manager->assets.find(asset->metadata.handle) != manager->assets.end()) {
			// Re-import
			// TODO: Add proper remove asset function
			delete manager->assets[asset->metadata.handle];
		}
		manager->assets[asset->metadata.handle] = asset;
	}

	UUID updateDirectoryAssets(AssetManager* manager, const std::string& path, UUID parent) {
		Directory* directory = static_cast<Directory*>(createAsset(manager, path, AssetType::Directory, parent));
		directory->runtimeData.loaded = true;

		// Register un-registered directories
		if (manager->registry.find(directory->metadata.path) == manager->registry.end()) {
			manager->registry.emplace(directory->metadata.path, directory->metadata);
		}

		// Add/replace directory asset
		if (manager->assets.find(directory->metadata.handle) != manager->assets.end()) {
			delete manager->assets[directory->metadata.handle];
		}
		manager->assets[directory->metadata.handle] = directory;

		// If we have a parent, add ourself as a child
		if (parent.isValid()) {
			static_cast<Directory*>(manager->assets.at(parent))->children.push_back(directory->metadata.handle);
		}

		// Itterate over all files in directory and recusivly update them
		for (auto& entry : std::filesystem::directory_iterator(path)) {
			if (entry.is_directory()) {
				updateDirectoryAssets(manager, entry.path().generic_string(), directory->metadata.handle);
			}
			else {
				importAsset(manager, entry.path().generic_string(), directory->metadata.handle);
			}
		}

		return directory->metadata.handle;
	}

	struct SortAssets {
		inline bool operator() (const std::pair<UUID, Asset*>& a, const std::pair<UUID, Asset*>& b) {
			std::string aFilename = a.second->runtimeData.filename;
			std::string bFilename = b.second->runtimeData.filename;

			std::for_each(aFilename.begin(), aFilename.end(), [](char& c) { c = std::tolower(c); });
			std::for_each(bFilename.begin(), bFilename.end(), [](char& c) { c = std::tolower(c); });
			return a.second->metadata.type < b.second->metadata.type || a.second->metadata.type == b.second->metadata.type && aFilename < bFilename;
		}
	};

	void updateAssetRegistry(AssetManager* manager) {
		manager->sortedAssets.clear();
		for (auto& [id, asset] : manager->assets) {
			manager->sortedAssets.push_back(std::make_pair(id, asset));
		}
		std::sort(manager->sortedAssets.begin(), manager->sortedAssets.end(), SortAssets());

		// Clear registry
		for (auto it = manager->registry.begin(); it != manager->registry.end();) {
			if (manager->assets.find(it->second.handle) == manager->assets.end()) {
				it = manager->registry.erase(it);
			}
			else {
				it++;
			}
		}
	}

}
