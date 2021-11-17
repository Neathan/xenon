#include "asset_viewer.h"

#include <imgui.h>

namespace xe {

	bool drawAsset(AssetManager* manager, Asset* asset, bool isBack = false, bool isSelected = false) {
		// TODO(Neathan): Find a good way to store asset references
		static Texture* fileIcon = getAsset<Texture>(manager, "assets/icons/file.png");
		static Texture* directoryIcon = getAsset<Texture>(manager, "assets/icons/directory.png");
		static Texture* modelIcon = getAsset<Texture>(manager, "assets/icons/model.png");
		static Texture* textureIcon = getAsset<Texture>(manager, "assets/icons/texture.png");
		static Texture* backIcon = getAsset<Texture>(manager, "assets/icons/back_folder.png");

		// Push custom unique id
		ImGui::PushID(asset->metadata.handle);
		ImGui::BeginGroup();

		// Select file icon
		GLuint textureID = 0;
		switch(asset->metadata.type) {
		case AssetType::Directory:
			textureID = isBack ? backIcon->textureID : directoryIcon->textureID;
			break;
		case AssetType::Model:
			textureID = modelIcon->textureID;
			break;
		case AssetType::Texture:
			textureID = textureIcon->textureID;
			break;
		default:
			textureID = fileIcon->textureID;
			break;
		}

		// File image button
		bool clicked = ImGui::ImageButton((ImTextureID)textureID, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
		if (!isBack && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::Text(asset->metadata.path.c_str());
			ImGui::SetDragDropPayload("asset", &asset->metadata.handle, sizeof(UUID));
			ImGui::EndDragDropSource();
		}
		ImGui::SameLine();
		
		// File information
		ImGui::BeginGroup();
		ImGui::TextWrapped(asset->runtimeData.filename.c_str());
		ImGui::Text("Type: %i", asset->metadata.type);
		ImGui::Text("Loaded: %i", asset->runtimeData.loaded);
		ImGui::EndGroup();

		ImGui::EndGroup();
		ImGui::PopID();
		return clicked;
	}

	void drawAssetViewer(EditorData* data) {
		if (ImGui::Begin("Assets")) {
			float columnWidth = 200.0f;
			// TODO(Neathan): Explore using tables API instead
			ImGui::Columns((int)(ImGui::GetContentRegionAvail().x / columnWidth), "AssetViewerColumns", false);

			// Create back button
			if (data->assetDir->runtimeData.parent.isValid()) {
				Directory* asset = getAsset<Directory>(data->assetManager, data->assetDir->runtimeData.parent);
				if (drawAsset(data->assetManager, asset, true)) {
					data->assetDir = asset;
				}
				ImGui::NextColumn();
			}

			// Create assets
			for (auto& [id, asset] : data->assetManager->sortedAssets) {
				// Skip assets not in current directory
				if (asset->runtimeData.parent != data->assetDir->metadata.handle) {
					continue;
				}

				// Draw asset and check if it was pressed
				if (drawAsset(data->assetManager, asset, false, asset == data->selectedAsset)) {
					// If directory is pressed move into it
					if (asset->metadata.type == AssetType::Directory) {
						data->assetDir = static_cast<Directory*>(asset);
					}

					data->selectedAsset = asset;
					data->selectedEntityID = UUID::None();
				}
				ImGui::NextColumn();
			}

			// Reset columns
			ImGui::Columns();
		}
		ImGui::End();
	}

}