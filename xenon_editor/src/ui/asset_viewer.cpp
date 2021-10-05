#include "asset_viewer.h"

#include <imgui.h>

namespace xe {

	bool drawAsset(AssetManager* manager, Asset* asset, bool isBack = false, bool isSelected = false) {
		static Texture* fileIcon = getAsset<Texture>(manager, "assets/icons/file.png");
		static Texture* directoryIcon = getAsset<Texture>(manager, "assets/icons/directory.png");
		static Texture* modelIcon = getAsset<Texture>(manager, "assets/icons/model.png");
		static Texture* textureIcon = getAsset<Texture>(manager, "assets/icons/texture.png");
		static Texture* backIcon = getAsset<Texture>(manager, "assets/icons/back_folder.png");

		ImGui::PushID(asset->metadata.handle);
		ImGui::BeginGroup();

		GLuint textureID = 0;
		if (asset->metadata.type == AssetType::Directory) {
			if (!isBack) {
				textureID = directoryIcon->textureID;
			}
			else {
				textureID = backIcon->textureID;
			}
		}
		else if (asset->metadata.type == AssetType::Model) {
			textureID = modelIcon->textureID;
		}
		else if (asset->metadata.type == AssetType::Texture) {
			textureID = textureIcon->textureID;
		}
		else {
			textureID = fileIcon->textureID;
		}

		bool clicked = ImGui::ImageButton((ImTextureID)textureID, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
		if (!isBack && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::Text(asset->metadata.path.c_str());
			ImGui::SetDragDropPayload("asset", &asset->metadata.handle, sizeof(UUID));
			ImGui::EndDragDropSource();
		}

		ImGui::SameLine();
		ImGui::BeginGroup();
		{
			ImGui::TextWrapped(asset->runtimeData.filename.c_str());
			ImGui::Text("Type: %i", asset->metadata.type);
			ImGui::Text("Loaded: %b", asset->runtimeData.loaded);
		}
		ImGui::EndGroup();

		ImGui::EndGroup();
		ImGui::PopID();
		return clicked;
	}

	void drawAssetViewer(EditorData* data) {
		if (ImGui::Begin("Assets")) {
			size_t rowIndex = 0;
			
			ImGui::Columns((int)(ImGui::GetContentRegionAvail().x / 200.0f), "AssetViewerColumns", false);

			// Create back button
			if (data->assetViewerDirectory->runtimeData.parent.isValid()) {
				Directory* asset = getAsset<Directory>(data->assetManager, data->assetViewerDirectory->runtimeData.parent);
				if (drawAsset(data->assetManager, asset, true)) {
					data->assetViewerDirectory = asset;
				}
				ImGui::NextColumn();
			}

			// Create assets
			for (auto& [id, asset] : data->assetManager->sortedAssets) {
				if (asset->runtimeData.parent != data->assetViewerDirectory->metadata.handle) {
					continue;
				}

				if (drawAsset(data->assetManager, asset, false, asset == data->selectedAsset)) {
					if (asset->metadata.type == AssetType::Directory) {
						data->assetViewerDirectory = static_cast<Directory*>(asset);
					}
					data->selectedAsset = asset;
					data->selectedEntityID = UUID::None();
				}
				ImGui::NextColumn();
			}
			ImGui::Columns();
			
		}
		ImGui::End();
	}

}