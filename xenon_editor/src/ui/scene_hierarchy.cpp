#include "scene_hierarchy.h"

#include <imgui.h>
#include <map>

namespace xe {
	
	struct MoveAction {
		UUID source;
		UUID target;
	};

	void drawHierarchyItemPayloadTarget(UUID id, std::vector<MoveAction>& moveActions) {
		if (ImGui::BeginDragDropTarget()) {
			auto data = ImGui::AcceptDragDropPayload("hierarchyItem");
			if (data) {
				UUID handle = *(UUID*)data->Data;
				if (handle != id) {
					moveActions.push_back(MoveAction{ handle, id });
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void drawHirarchyContextMenuContent(Scene* scene, UUID contextEntity, UUID& selectedEntityID) {
		if (ImGui::BeginMenu("Add")) {
			if (ImGui::MenuItem("Empty")) {
				Entity entity = createEntity(scene);

				entity.getComponent<TransformComponent>().parent = contextEntity;

				selectedEntityID = entity.getComponent<IdentityComponent>().uuid;
			}
			if (ImGui::MenuItem("Sphere")) {
				Entity entity = createEntity(scene, "Sphere");
				entity.getComponent<TransformComponent>().parent = contextEntity;
				
				ModelComponent& modelComponent = entity.addComponent<ModelComponent>();
				setModelComponentModel(modelComponent, loadModel("assets/models/0.1.sphere.glb"));

				selectedEntityID = entity.getComponent<IdentityComponent>().uuid;
			}
			if (ImGui::MenuItem("Light")) {
				Entity entity = createEntity(scene, "Light");

				entity.getComponent<TransformComponent>().parent = contextEntity;
				entity.addComponent<PointLightComponent>(glm::vec3(10, 10, 10));

				selectedEntityID = entity.getComponent<IdentityComponent>().uuid;
			}

			ImGui::EndMenu();
		}
	}

	void drawHierarchyItem(Scene* scene, UUID id, const std::map<UUID, std::vector<UUID>>& children, std::vector<MoveAction>& moveActions, UUID& selectedItem, bool isRoot = false) {
		ImGui::PushID(id); // Avoid colliding names

		IdentityComponent& identity = getEntityFromID(scene, id).getComponent<IdentityComponent>();

		bool hasChildren = children.find(id) != children.end();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
		if (!isRoot || !hasChildren) {
			flags |= ImGuiTreeNodeFlags_Leaf;
		}
		else {
			flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
		}
		if (selectedItem == id) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool open = ImGui::TreeNodeEx(identity.name.c_str(), flags);

		if (ImGui::BeginPopupContextItem("hierarchyContextMenu")) {
			drawHirarchyContextMenuContent(scene, id, selectedItem);
			ImGui::EndPopup();
		}

		// Drag and drop
		if (ImGui::BeginDragDropSource()) {
			ImGui::Text(identity.name.c_str());
			ImGui::SetDragDropPayload("hierarchyItem", &id, sizeof(UUID));
			ImGui::EndDragDropSource();
		}
		drawHierarchyItemPayloadTarget(id, moveActions);

		// Selection
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			selectedItem = id;
		}

		// Draw children
		if (open) {
			if (hasChildren) {
				for (const auto& child : children.at(id)) {
					drawHierarchyItem(scene, child, children, moveActions, selectedItem);
				}
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	bool isRelationRecursive(Scene* scene, UUID source, UUID parent) {
		if (!parent.isValid()) {
			return false;
		}
		else if (parent == source) {
			return true;
		}
		isRelationRecursive(scene, source, getEntityFromID(scene, parent).getComponent<TransformComponent>().parent);
	}

	void drawHierarchy(Scene* scene, UUID& selectedItem) {
		// Push style color so selected item is visible
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.25f, 0.25f, 1.00f));

		if (ImGui::Begin("Scene heirarchy")) {
			auto view = scene->registry.view<IdentityComponent, TransformComponent>();

			std::map<UUID, std::vector<UUID>> children;
			std::vector<UUID> roots;

			std::vector<MoveAction> moveActions;

			for (const auto [entity, identity, transform] : view.each()) {
				if (transform.parent.isValid()) {
					children[transform.parent].emplace_back(identity.uuid);
				}
				else {
					roots.emplace_back(identity.uuid);
				}
			}

			for (const UUID& id : roots) {
				drawHierarchyItem(scene, id, children, moveActions, selectedItem, true);
			}

			ImGui::Dummy(ImGui::GetContentRegionAvail());
			drawHierarchyItemPayloadTarget(UUID::None(), moveActions);
			if (ImGui::BeginPopupContextItem("hierarchyContextMenu")) {
				drawHirarchyContextMenuContent(scene, UUID::None(), selectedItem);
				ImGui::EndPopup();
			}

			// Move items
			for (const auto& moveAction : moveActions) {
				if (!isRelationRecursive(scene, moveAction.source, moveAction.target)) {
					// Keep world position
					Entity sourceEntity = getEntityFromID(scene, moveAction.source);
					TransformComponent& transformComponent = sourceEntity.getComponent<TransformComponent>();

					glm::mat4 worldMatrix = getWorldMatrix(sourceEntity);

					if (moveAction.target.isValid()) {
						Entity targetEntity = getEntityFromID(scene, moveAction.target);
						glm::mat4 newLocalMatrix = glm::inverse(getWorldMatrix(targetEntity)) * worldMatrix;
						transformComponent.matrix = newLocalMatrix;
					}
					else {
						transformComponent.matrix = worldMatrix;
					}

					// Set parent
					transformComponent.parent = moveAction.target;
				}
			}
		}
		ImGui::End();

		ImGui::PopStyleColor();
	}

}