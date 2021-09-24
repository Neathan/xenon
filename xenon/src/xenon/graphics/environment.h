#pragma once

#include "xenon/graphics/texture.h"

namespace xe {

	struct Environment {
		std::shared_ptr<Texture> environmentCubemap = nullptr;
		std::shared_ptr<Texture> irradianceMap = nullptr;
		std::shared_ptr<Texture> radianceMap = nullptr;
	};

}
