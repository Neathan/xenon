#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

#include "xenon/core/asset.h"
#include "xenon/core/asset_manager.h"

namespace ImGui {

	bool InputVector2(const char* label, glm::vec2& vector, ImGuiInputTextFlags flags = 0);
	bool InputVector3(const char* label, glm::vec3& vector, ImGuiInputTextFlags flags = 0);
	bool InputVector4(const char* label, glm::vec4& vector, ImGuiInputTextFlags flags = 0);

	bool InputVector2(const char* label, float* vector, ImGuiInputTextFlags flags = 0);
	bool InputVector3(const char* label, float* vector, ImGuiInputTextFlags flags = 0);
	bool InputVector4(const char* label, float* vector, ImGuiInputTextFlags flags = 0);

	template<typename T>
	bool InputAsset(const char* label, xe::AssetManager* manager, xe::AssetType acceptedType, T* originalAsset, T** targetAsset) {
		// Asset path
		ImGui::InputText(label, originalAsset ? &originalAsset->metadata.path : &std::string(), ImGuiInputTextFlags_ReadOnly);

		// Drop target accepting specified type
		if (ImGui::BeginDragDropTarget()) {
			auto data = ImGui::AcceptDragDropPayload("asset");
			if (data) {
				xe::UUID handle = *(xe::UUID*)data->Data;
				xe::Asset* asset = xe::getAsset<xe::Asset>(manager, handle);

				if (asset->metadata.type == acceptedType) {
					*targetAsset = static_cast<T*>(asset);
					return true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return false;
	}

	template<typename T>
	bool InputAsset(const char* label, xe::AssetManager* manager, xe::AssetType acceptedType, T** targetAsset) {
		return InputAsset<T>(label, manager, acceptedType, *targetAsset, targetAsset);
	}


}
