#include "imgui_components.h"

#include <glm/gtc/type_ptr.hpp>
#include <misc/cpp/imgui_stdlib.h>

namespace ImGui {

	bool InputVector2(const char* label, glm::vec2& vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat2(label, glm::value_ptr(vector), "%.3f", flags);
	}

	bool InputVector3(const char* label, glm::vec3& vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat3(label, glm::value_ptr(vector), "%.3f", flags);
	}

	bool InputVector4(const char* label, glm::vec4& vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat4(label, glm::value_ptr(vector), "%.3f", flags);
	}

	bool InputVector2(const char* label, float* vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat2(label, vector, "%.3f", flags);
	}

	bool InputVector3(const char* label, float* vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat3(label, vector, "%.3f", flags);
	}

	bool InputVector4(const char* label, float* vector, ImGuiInputTextFlags flags) {
		return ImGui::InputFloat4(label, vector, "%.3f", flags);
	}

}
