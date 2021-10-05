#pragma once

#include "xenon/graphics/texture.h"

namespace xe {

	struct Environment {
		Texture* environmentCubemap = nullptr;
		Texture* irradianceMap = nullptr;
		Texture* radianceMap = nullptr;
	};

}
