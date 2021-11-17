#include "asset_serializer.h"

#include "xenon/graphics/model_loader.h"
#include "xenon/graphics/texture.h"

namespace xe {

	bool loadModelAsset(AssetManager* manager, Asset** asset) {
		const Asset* sourceAsset = *asset;
		*asset = loadModel(manager, (*asset)->metadata.path);
		copyAssetMetadata(sourceAsset, *asset);
		return true;
	}

	bool loadTextureAsset(AssetManager* manager, Asset** asset) {
		const Asset* sourceAsset = *asset;
		*asset = loadTexture((*asset)->metadata.path);
		copyAssetMetadata(sourceAsset, *asset);
		return true;
	}

}

