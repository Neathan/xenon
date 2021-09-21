#pragma once

#include <string>

#include "xenon/graphics/environment.h"

namespace xe { 

	Environment loadIBLCubemap(const std::string& path, int resolution = 512, int iblResolution = 64);


}
