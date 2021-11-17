#include "inspector.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>

#include "ui/imgui_components.h"

namespace xe {

	bool beginComponent(const char* label, bool collapsingHeader = true) {
		bool isHeaderOpen = collapsingHeader && ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
		if (isHeaderOpen || !collapsingHeader) {
			if (ImGui::BeginTable("InspectorTable", 2, ImGuiTableFlags_SizingFixedFit)) {
				ImGui::TableSetupColumn("###Labels", ImGuiTableColumnFlags_WidthStretch, 0.2);
				ImGui::TableSetupColumn("###Field", ImGuiTableColumnFlags_WidthStretch, 0.8);
				return true;
			}
		}
		return false;
	}

	void endComponent(bool isOpen) {
		if (isOpen) {
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0, 4));
		ImGui::Separator();
	}

	void beginField(const char* label, bool pushFullWidth = true) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text(label);
		ImGui::TableNextColumn();

		if (pushFullWidth) {
			ImGui::PushItemWidth(-1);
		}
	}

	void endField(bool pushedFullWidth = true) {
		if (pushedFullWidth) {
			ImGui::PopItemWidth();
		}
	}

	void drawIdentityComponent(IdentityComponent& identity) {
		bool open = beginComponent("Identity", false);
		if (open) {
			beginField("Name");
			ImGui::InputText("###Name", &identity.name);
			endField();

			beginField("UUID");
			ImGui::InputText("###UUID", &std::to_string(identity.uuid), ImGuiInputTextFlags_ReadOnly);
			endField();
		}
		endComponent(open);
	}

	void drawTransformComponent(TransformComponent& transform) {
		bool open = beginComponent("Transform");
		if (open) {
			beginField("Position");
			ImGui::InputVector3("###Position", glm::value_ptr(transform.matrix[3]));
			endField();

			beginField("Scale");
			ImGui::InputVector3("###Scale", glm::vec3());
			endField();
		}
		endComponent(open);
	}

	void drawPointLightComponent(PointLightComponent& pointLight) {
		bool open = beginComponent("Point Light");
		if (open) {
			beginField("Color");
			if (ImGui::ColorButton("Color", ImVec4(pointLight.color.r, pointLight.color.g, pointLight.color.b, 1))) {
				ImGui::OpenPopup("PointLightColorPicker");
			}
			endField();
			if (ImGui::BeginPopup("PointLightColorPicker")) {
				ImGui::ColorPicker3("###Color", glm::value_ptr(pointLight.color));
				ImGui::EndPopup();
			}
		}
		endComponent(open);
	}

	void drawScriptComponent(EditorData* data, ScriptComponent& script) {
		bool open = beginComponent("Script");
		if (open) {
			Entity selectedEntity = getEntityFromID(getActiveScene(data), data->selectedEntityID);

			beginField("Name");
			std::string lastModuleName = script.moduleName; // TODO: Find better way to do this
			if (ImGui::InputText("###Name", &script.moduleName)) {
				if (moduleExists(data->scriptContext, lastModuleName)) {
					cleanScriptEntity(data->scriptContext, selectedEntity);
				}
				if (moduleExists(data->scriptContext, script.moduleName)) {
					loadScriptEntity(data->scriptContext, selectedEntity);
				}
			}
			endField();

			// Public fields
			if (moduleExists(data->scriptContext, script.moduleName)) {
				for (auto& [name, field] : getInstanceFields(data->scriptContext, selectedEntity)) {
					bool isRuntime = data->playState != PlayModeState::Edit && isRuntimeAvailable(field);

					beginField(name.c_str());
					if (field.type == FieldType::Int) {
						int value = isRuntime ? getRuntimeValue<int>(field) : getStoredValue<int>(field);
						if (ImGui::InputInt(field.name.c_str(), &value)) {
							isRuntime ? setRuntimeValue(field, value) : setStoredValue(field, value);
						}
					}
					else if (field.type == FieldType::Float) {
						float value = isRuntime ? getRuntimeValue<float>(field) : getStoredValue<float>(field);
						if (ImGui::InputFloat(field.name.c_str(), &value)) {
							isRuntime ? setRuntimeValue(field, value) : setStoredValue(field, value);
						}
					}
					else if (field.type == FieldType::Vec2) {
						glm::vec2 value = isRuntime ? getRuntimeValue<glm::vec2>(field) : getStoredValue<glm::vec2>(field);
						if (ImGui::InputVector2(field.name.c_str(), glm::value_ptr(value))) {
							isRuntime ? setRuntimeValue(field, value) : setStoredValue(field, value);
						}
					}
					else if (field.type == FieldType::Vec3) {
						glm::vec3 value = isRuntime ? getRuntimeValue<glm::vec3>(field) : getStoredValue<glm::vec3>(field);
						if (ImGui::InputVector3(field.name.c_str(), glm::value_ptr(value))) {
							isRuntime ? setRuntimeValue(field, value) : setStoredValue(field, value);
						}
					}
					else if (field.type == FieldType::Vec4) {
						glm::vec4 value = isRuntime ? getRuntimeValue<glm::vec4>(field) : getStoredValue<glm::vec4>(field);
						if (ImGui::InputVector4(field.name.c_str(), glm::value_ptr(value))) {
							isRuntime ? setRuntimeValue(field, value) : setStoredValue(field, value);
						}
					}
					endField();
				}
			}
		}
		endComponent(open);
	}

	void drawModelComponent(AssetManager* manager, ModelComponent& modelComponent) {
		bool open = beginComponent("Model");
		if (open) {
			beginField("Model");
			Asset* asset = nullptr;
			if (ImGui::InputAsset<Asset>("###modelPath", manager, AssetType::Model, modelComponent.model, &asset)) {
				if (modelComponent.model) {
					// TODO: This should be replaced with asset manager functionality
					destroyModel(modelComponent.model);
				}
				setModelComponentModel(modelComponent, static_cast<Model*>(asset));
				modelComponent.animation = ModelAnimation();
			}
			endField();

			// Animation
			if (modelComponent.model) {
				beginField("Animation index");
				if (ImGui::DragInt("###Animation index", &modelComponent.animation.animationIndex, 1, -1, modelComponent.model->animations.size() - 1)) {
					modelComponent.animation.isDirty = true;
				}
				endField();
			}
		}
		endComponent(open);


		// Material
		if (modelComponent.model && !modelComponent.model->materials.empty()) {
			bool open = beginComponent("Materials");
			if (open) {
				for (size_t i = 0; i < modelComponent.model->materials.size(); ++i) {
					auto& material = modelComponent.model->materials[i];
					ImGui::BeginGroup();
					ImGui::PushID(i);

					beginField("Name");
					ImGui::InputText("###Name", &material.name);
					endField();

					beginField("Base color");
					glm::vec4& color = material.pbrMetallicRoughness.baseColorFactor;
					if (ImGui::ColorButton("Base color", ImVec4(color.r, color.g, color.b, 1))) {
						ImGui::OpenPopup("BaseColorPicker");
					}
					// Color picker
					if (ImGui::BeginPopup("BaseColorPicker")) {
						ImGui::ColorPicker3("###BaseColor", glm::value_ptr(color));
						ImGui::EndPopup();
					}
					endField();

					beginField("Albedo");
					ImGui::InputAsset("###Albedo", manager, AssetType::Texture, (Asset**)&material.pbrMetallicRoughness.baseColorTexture);
					endField();

					beginField("Metallic");
					ImGui::InputFloat("###Metallic", &material.pbrMetallicRoughness.metallicFactor);
					endField();

					beginField("Roughness");
					ImGui::InputFloat("###Roughness", &material.pbrMetallicRoughness.roughnessFactor);
					endField();

					beginField("Metallic roughness");
					ImGui::InputAsset("###Metallic roughness", manager, AssetType::Texture, (Asset**)&material.pbrMetallicRoughness.metallicRoughnessTexture);
					endField();

					beginField("Normal");
					ImGui::InputAsset("###Normal", manager, AssetType::Texture, (Asset**)&material.normalTexture);
					endField();

					beginField("Occlusion");
					ImGui::InputAsset("###Occlusion", manager, AssetType::Texture, (Asset**)&material.occlusionTexture);
					endField();

					beginField("Emissive");
					ImGui::InputAsset("###Emissive", manager, AssetType::Texture, (Asset**)&material.emissiveTexture);
					endField();

					beginField("Emissive factor");
					ImGui::InputVector3("###Emissive factor", material.emissiveFactor);
					endField();

					// TODO: Alpha mode

					beginField("Alpha cutoff");
					ImGui::InputFloat("###Alpha cutoff", &material.alphaCutoff);
					endField();

					beginField("Double sided");
					ImGui::Checkbox("###Double sided", &material.doubleSided);
					endField();

					ImGui::PopID();
					ImGui::EndGroup();
				}
			}
			endComponent(open);
		}
	}

	void drawCameraComponent(EditorData* data, Entity entity, CameraComponent& cameraComponent) {
		bool open = beginComponent("Camera");
		if (open) {
			beginField("Main camera");
			Scene* scene = getActiveScene(data);
			Camera& camera = cameraComponent.camera;
			UUID entityID = getEntityID(entity);
			bool isMainCamera = scene->mainCameraEntityID == entityID;

			if (ImGui::Checkbox("###Main camera", &isMainCamera)) {
				if (isMainCamera) {
					scene->mainCameraEntityID = entityID;
				}
				else {
					scene->mainCameraEntityID = UUID::None();
				}
			}
			endField();

			beginField("FOV");
			ImGui::DragFloat("###FOV", &cameraComponent.fov, 1.0f, 10, 180);
			endField();

			beginField("Width");
			ImGui::InputInt("###Width", &cameraComponent.width);
			endField();

			beginField("Height");
			ImGui::InputInt("###Height", &cameraComponent.height);
			endField();

			// TODO: Projection type
		}
		endComponent(open);
	}

	void drawComponentSelector(Entity entity) {
		static std::vector<std::string> componentNames = {
			"ModelComponent",
			"ScriptComponent",
			"PointLightComponent",
			"CameraComponent"
		};
		if (ImGui::BeginPopup("ComponentSelector")) {
			static std::string selected = "";
			static ImGuiTextFilter filter;
			static std::string errorMsg = "";
			filter.Draw("###Filter");

			if (ImGui::BeginListBox("###ComponentList")) {
				for (int i = 0; i < componentNames.size(); ++i) {
					if (filter.PassFilter(componentNames[i].c_str())) {
						bool isSelected = componentNames[i] == selected;
						if (ImGui::Selectable(componentNames[i].c_str(), isSelected)) {
							selected = componentNames[i];
							errorMsg = "";
						}
					}
				}
				ImGui::EndListBox();
			}

			if (ImGui::Button("Add")) {
				errorMsg = "";

				if (!selected.empty()) {
					if (selected == "ModelComponent" && !entity.hasComponent<ModelComponent>()) {
						entity.addComponent<ModelComponent>();
					}
					else if (selected == "ScriptComponent" && !entity.hasComponent<ScriptComponent>()) {
						entity.addComponent<ScriptComponent>();
					}
					else if (selected == "PointLightComponent" && !entity.hasComponent<PointLightComponent>()) {
						entity.addComponent<PointLightComponent>();
					}
					else if (selected == "CameraComponent" && !entity.hasComponent<CameraComponent>()) {
						entity.addComponent<CameraComponent>();
					}
					ImGui::CloseCurrentPopup();
					selected = "";
				}
				else {
					errorMsg = "Need to select component";
				}
			}
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1, 0, 0, 1), errorMsg.c_str());

			ImGui::EndPopup();
		}
	}

	void drawInspector(EditorData* data) {

		if (ImGui::Begin("Inspector")) {
			// Components
			if (data->selectedEntityID.isValid()) {
				Entity entity = getEntityFromID(getActiveScene(data), data->selectedEntityID);
				drawIdentityComponent(entity.getComponent<IdentityComponent>());

				drawTransformComponent(entity.getComponent<TransformComponent>());

				if (entity.hasComponent<CameraComponent>()) {
					drawCameraComponent(data, entity, entity.getComponent<CameraComponent>());
				}
				if (entity.hasComponent<ModelComponent>()) {
					drawModelComponent(data->assetManager, entity.getComponent<ModelComponent>());
				}
				if (entity.hasComponent<ScriptComponent>()) {
					drawScriptComponent(data, entity.getComponent<ScriptComponent>());
				}
				if (entity.hasComponent<PointLightComponent>()) {
					drawPointLightComponent(entity.getComponent<PointLightComponent>());
				}

				if (ImGui::Button("Add component")) {
					ImGui::OpenPopup("ComponentSelector");
				}
				drawComponentSelector(entity);
			}
			// Asset
			else if (data->selectedAsset != nullptr) {
				ImGui::Text(data->selectedAsset->runtimeData.filename.c_str());
				ImGui::Separator();

				bool open = beginComponent("AssetInspectorContent", false);

				if (open) {
					beginField("Asset UUID");
					ImGui::Text(std::to_string(data->selectedAsset->metadata.handle).c_str());
					endField();

					beginField("Asset path");
					ImGui::Text(data->selectedAsset->metadata.path.c_str());
					endField();

					beginField("Asset type");
					// TODO: Add readable type name
					ImGui::Text(std::to_string((int)data->selectedAsset->metadata.type).c_str());
					endField();

					beginField("Filename");
					ImGui::Text(data->selectedAsset->runtimeData.filename.c_str());
					endField();

					beginField("Extension");
					ImGui::Text(data->selectedAsset->runtimeData.extension.c_str());
					endField();

					beginField("Loaded");
					ImGui::BeginDisabled();
					ImGui::Checkbox("###Loaded", &data->selectedAsset->runtimeData.loaded);
					ImGui::EndDisabled();
					endField();

					beginField("Parent");
					ImGui::Text(std::to_string(data->selectedAsset->runtimeData.parent).c_str());
					endField();
				}

				endComponent(open);
			}
		}
		ImGui::End();
		

	}

}
