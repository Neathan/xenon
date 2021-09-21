#pragma once

#include "xenon/graphics/texture.h"

namespace xe {

	struct Environment {
		std::shared_ptr<Texture> iblTexture = nullptr;
		std::shared_ptr<Texture> iblCubemap = nullptr;
		std::shared_ptr<Texture> environmentCubemap = nullptr;
	};

}
