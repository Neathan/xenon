#include "animation.h"

namespace xe {

	Interpolation getInterpolation(const std::string& value) {
		if (value == "LINEAR") {
			return Interpolation::LINEAR;
		}
		else if (value == "STEP") {
			return Interpolation::STEP;
		}
		else { // "CUBICSPLINE"
			return Interpolation::CUBICSPLINE;
		}
	}

}
