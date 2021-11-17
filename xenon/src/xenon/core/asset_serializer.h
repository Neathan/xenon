#pragma once

#include "xenon/core/asset_manager.h"

namespace xe {
	
	// TODO(Neathan): Model serialization
	bool loadModelAsset(AssetManager* manager, Asset** asset);

	// TODO(Neathan): Texture serialization
	bool loadTextureAsset(AssetManager* manager, Asset** asset);

}
