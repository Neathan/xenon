#include "inspector.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "ui/imgui_components.h"

namespace xe {

	void drawIdentityComponent(IdentityComponent& identity) {
		ImGui::LabelText("UUID", "%llu", identity.uuid);
		ImGui::InputText("Name", &identity.name);
	}

	void drawTransformComponent(TransformComponent& transform) {
		ImGui::InputVector("Position", glm::vec3(transform.matrix[3]));
		ImGui::InputVector("Scale", glm::vec3());
	}

	void drawInspector(Scene* scene, UUID& selectedEntity) {

		if (ImGui::Begin("Inspector") && selectedEntity.isValid()) {
			Entity entity = getEntityFromID(scene, selectedEntity);

			drawIdentityComponent(entity.getComponent<IdentityComponent>());
			ImGui::Separator();

			drawTransformComponent(entity.getComponent<TransformComponent>());

		}
		ImGui::End();
		

	}

}
