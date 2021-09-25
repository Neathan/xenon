#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

namespace ImGui {

	bool InputVector(const char* label, glm::vec2& vector, ImGuiInputTextFlags flags = 0);
	bool InputVector(const char* label, glm::vec3& vector, ImGuiInputTextFlags flags = 0);
	bool InputVector(const char* label, glm::vec4& vector, ImGuiInputTextFlags flags = 0);

}
