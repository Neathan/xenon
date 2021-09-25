#include "imgui_components.h"

#include <glm/gtc/type_ptr.hpp>

namespace ImGui {

	bool InputVector(const char* label, glm::vec2& vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat2(label, glm::value_ptr(vector), "%.3f", flags);
	}

	bool InputVector(const char* label, glm::vec3& vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat3(label, glm::value_ptr(vector), "%.3f", flags);
	}

	bool InputVector(const char* label, glm::vec4& vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat4(label, glm::value_ptr(vector), "%.3f", flags);
	}

}
