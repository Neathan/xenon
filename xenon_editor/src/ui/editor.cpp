#include "editor.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_internal.h>

#include "ui/imgui_operators.h"

#include "ui/scene_hierarchy.h"
#include "ui/inspector.h"

namespace xe {

	EditorData* createEditor() {
		EditorData* editor = new EditorData();

		// Create PBR renderer and shader
		editor->pbrShader = loadShader("assets/pbr.vert", "assets/pbr.frag");
		editor->envShader = loadShader("assets/env.vert", "assets/env.frag");
		editor->renderer = createRenderer(editor->pbrShader, editor->envShader);

		/* NOTE: Only needed for runtime rendering. Only included for reference.
		// Create framebuffer renderer and shader
		Shader* framebufferShader = loadShader("assets/framebuffer.vert", "assets/framebuffer.frag");
		FramebufferRenderer* framebufferRenderer = createFramebufferRenderer(framebufferShader);
		*/

		// Create framebuffer to render to (multisampled)
		editor->framebuffer = createFramebuffer(1920, 1080, 16);
		editor->framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::DEPTH, 0));
		editor->framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->framebuffer);

		// Create framebuffer to blit to (resolved multisample)
		editor->displayedFramebuffer = createFramebuffer(1920, 1080, 1); // No AA
		editor->displayedFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->displayedFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->displayedFramebuffer);

		// Camera
		editor->camera = createOrbitCamera(1920, 1080);

		// Scene
		editor->scene = createScene();

		return editor;
	}

	void destroyEditor(EditorData* data) {
		destroyScene(data->scene);

		destroyFramebuffer(data->displayedFramebuffer);
		destroyFramebuffer(data->framebuffer);

		destroyRenderer(data->renderer);
		destroyShader(data->envShader);
		destroyShader(data->pbrShader);

		delete data;
	}


	float drawMenuBar(EditorData* data) {
		float menuHeight = 0;
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				ImGui::EndMenu();
			}

			menuHeight = ImGui::GetWindowSize().y;
			ImGui::EndMainMenuBar();
		}
		return menuHeight;
	}

	void drawApplication(EditorData* data, float menuHeight) {
		ImGuiIO& io = ImGui::GetIO();
		const ImVec2 applicationSize = ImVec2(io.DisplaySize.x, io.DisplaySize.y - menuHeight);
		ImGui::SetNextWindowPos(ImVec2(0, menuHeight));
		ImGui::SetNextWindowSize(applicationSize);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus;

		if (ImGui::Begin("ApplicationRoot", nullptr, flags)) {
			ImGuiID applicationRootDockspaceID = ImGui::GetID("ApplicationRootDockspace");
			ImGui::DockSpace(applicationRootDockspaceID);
			// TODO: Build UI
		}
		ImGui::End();
	}

	void drawGizmo(EditorData* data) {
		if (data->selectedEntityID.isValid()) {
			float snap = 0.1f;
			ImGuizmo::SetRect(data->sceneViewportPos.x, data->sceneViewportPos.y, data->sceneViewportSize.x, data->sceneViewportSize.y);
			ImGuizmo::SetGizmoSizeClipSpace(0.05f * 1080.0f / data->sceneViewportSize.y);

			Entity selectedEntity = getEntityFromID(data->scene, data->selectedEntityID);

			TransformComponent& selectedTransform = selectedEntity.getComponent<TransformComponent>();
			glm::mat4 worldMatrix = getWorldMatrix(selectedEntity);

			BoundingBox bounds = BoundingBox{ glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1) };
			if (selectedEntity.hasComponent<ModelComponent>()) {
				ModelComponent& modelComponent = selectedEntity.getComponent<ModelComponent>();
				if (modelComponent.model) {
					bounds = modelComponent.model->bounds;
				}
			}

			bounds = bounds;
			glm::mat4 matrixBefore = worldMatrix;

			ImGuizmo::Manipulate(glm::value_ptr(data->camera.inverseTransform), glm::value_ptr(data->camera.projection),
				data->editOperation, data->editMode, glm::value_ptr(worldMatrix), NULL, Input::isKeyHeld(GLFW_KEY_LEFT_SHIFT) ? &snap : NULL, (float*)&bounds);

			// TODO: Replace when this open issue has been solved: https://github.com/CedricGuillemet/ImGuizmo/issues/201
			if (worldMatrix != matrixBefore) {
				selectedTransform.matrix = toLocalMatrix(worldMatrix, selectedEntity);
			}
		}
	}

	void drawViewport(EditorData* data) {
		if (ImGui::Begin("Scene")) {
			// Check if windows size changed
			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size != data->sceneViewportSize) {
				data->sceneViewportSizeChanged = true;
				data->sceneViewportSize = size;
			}

			data->sceneViewportPos = ImGui::GetCursorScreenPos();
			const ImVec2 viewportEnd = data->sceneViewportPos + data->sceneViewportSize;

			// Draw framebuffer
			ImGui::Image((ImTextureID)data->displayedFramebuffer->attachments.at(GL_COLOR_ATTACHMENT0).texture->textureID, data->sceneViewportSize, ImVec2(0, 1), ImVec2(1, 0));
			
			// Create hole for inputs
			ImGui::SetWindowHitTestHole(ImGui::GetCurrentWindow(), data->sceneViewportPos, data->sceneViewportSize);

			// Create clip rect to keep gizmo inside framebuffer image
			ImGui::PushClipRect(data->sceneViewportPos, viewportEnd, true);
			drawGizmo(data);
			ImGui::PopClipRect();

			// Picking
			ImVec2 mp = ImGui::GetMousePos();
			bool hovered = data->sceneViewportPos.x <= mp.x && mp.x <= viewportEnd.x && data->sceneViewportPos.y <= mp.y && mp.y <= viewportEnd.y;

			if (hovered && !ImGuizmo::IsUsing() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				bindFramebuffer(*data->displayedFramebuffer);
				bindShader(*data->renderer->shader);
				uint32_t objectID;
				glReadBuffer(GL_COLOR_ATTACHMENT1);
				glReadPixels(mp.x - data->sceneViewportPos.x, data->sceneViewportSize.y - (mp.y - data->sceneViewportPos.y), 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &objectID);
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				data->selectedEntityID = objectID;
				unbindShader();
				unbindFramebuffer();
			}
		}
		ImGui::End();
	}

	void drawEditor(EditorData* data) {
		float menuHeight = drawMenuBar(data);
		drawApplication(data, menuHeight);
		drawViewport(data);
		drawHierarchy(data->scene, data->selectedEntityID);
		drawInspector(data->scene, data->selectedEntityID);
	}

}
