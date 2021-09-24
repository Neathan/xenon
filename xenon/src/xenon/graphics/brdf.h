#pragma once

#include "xenon/graphics/texture.h"

namespace xe {

	std::shared_ptr<Texture> generateBRDFLUT(unsigned int width, unsigned int height);

}
