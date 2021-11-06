#pragma once

#include <map>
#include <vector>
#include <string>

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

namespace xe {

	enum class Interpolation : uint8_t {
		LINEAR,
		STEP,
		CUBICSPLINE
	};

	template<typename T>
	struct AnimationTransformation {
		float time;
		T value;
		Interpolation interpolation;
	};

	Interpolation getInterpolation(const std::string& value);

	struct NodeAnimation {
		std::vector<AnimationTransformation<float>> weight;
		std::vector<AnimationTransformation<glm::vec3>> translation;
		std::vector<AnimationTransformation<glm::quat>> rotation;
		std::vector<AnimationTransformation<glm::vec3>> scale;
	};

	struct Animation {
		int keyFrames = 0;
		float endTime = 0;
		std::map<uint16_t, NodeAnimation> nodeAnimations;
		std::string name;
	};

}
