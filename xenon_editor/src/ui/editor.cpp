#include "editor.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_internal.h>

#include "ui/imgui_operators.h"

#include "ui/scene_hierarchy.h"
#include "ui/inspector.h"
#include "ui/asset_viewer.h"

namespace xe {

	EditorData* createEditor(const std::string& projectFolder) {
		EditorData* editor = new EditorData();

		editor->assetManager = createAssetManager(projectFolder);

		// Create PBR renderer and shader
		editor->pbrShader = loadShader("assets/shaders/pbr.vert", "assets/shaders/pbr.frag");
		editor->envShader = loadShader("assets/shaders/env.vert", "assets/shaders/env.frag");
		editor->gridShader = loadShader("assets/shaders/grid.vert", "assets/shaders/grid.frag");
		editor->renderer = createRenderer(editor->pbrShader, editor->envShader);

		/* NOTE: Only needed for runtime rendering. Only included for reference.
		// Create framebuffer renderer and shader
		Shader* framebufferShader = loadShader("assets/framebuffer.vert", "assets/framebuffer.frag");
		FramebufferRenderer* framebufferRenderer = createFramebufferRenderer(framebufferShader);
		*/

		// Create scene framebuffer to render to (multi sampled)
		editor->editorSourceFramebuffer = createFramebuffer(1920, 1080, 16);
		editor->editorSourceFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->editorSourceFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::DEPTH, 0));
		editor->editorSourceFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->editorSourceFramebuffer);

		// Create scene framebuffer to blit to (resolved multi sample)
		editor->editorFramebuffer = createFramebuffer(1920, 1080, 1); // No AA
		editor->editorFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->editorFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->editorFramebuffer);

		// Create game framebuffer to render to (multi sampled)
		editor->gameSourceFramebuffer = createFramebuffer(1920, 1080, 16);
		editor->gameSourceFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->gameSourceFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::DEPTH, 0));
		editor->gameSourceFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->gameSourceFramebuffer);

		// Create game framebuffer to blit to (resolved multi sample)
		editor->gameFramebuffer = createFramebuffer(1920, 1080, 1); // No AA
		editor->gameFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->gameFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->gameFramebuffer);

		// Grid model
		editor->gridModel = generatePlaneModel(1, 1, GeneratorDirection::FRONT);

		// Camera
		editor->camera = createOrbitCamera(1920, 1080);

		// Scene
		editor->scene = createScene();
		editor->runtimeScene = nullptr;

		// Script context
		// TODO: Set to project name
		editor->scriptContext = createScriptContext("XenonProjectDomain");
		editor->scriptContext->scene = editor->scene;

		// Asset viewer
		editor->assetViewerDirectory = getAsset<Directory>(editor->assetManager, projectFolder, false);

		return editor;
	}

	void destroyEditor(EditorData* data) {
		destroyScriptContext(data->scriptContext);

		destroyScene(data->scene);
		if (data->runtimeScene) {
			destroyScene(data->runtimeScene);
		}

		destroyModel(data->gridModel);

		destroyFramebuffer(data->gameFramebuffer);
		destroyFramebuffer(data->gameSourceFramebuffer);

		destroyFramebuffer(data->editorFramebuffer);
		destroyFramebuffer(data->editorSourceFramebuffer);

		destroyRenderer(data->renderer);
		destroyShader(data->gridShader);
		destroyShader(data->envShader);
		destroyShader(data->pbrShader);

		destroyAssetManager(data->assetManager);

		delete data;
	}


	float drawMenuBar(EditorData* data) {
		float menuHeight = 0;
		// Push style to make menu seamless
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		if (ImGui::BeginMainMenuBar()) {
			// Pop style
			ImGui::PopStyleVar();

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

		// Push style
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		if (ImGui::Begin("ApplicationRoot", nullptr, flags)) {
			ImGui::PopStyleVar(3); // Pop style padding

			ImGuiID applicationRootDockspaceID = ImGui::GetID("ApplicationRootDockspace");
			ImGui::DockSpace(applicationRootDockspaceID);
			// TODO: Build UI
		}
		ImGui::End();

		// Pop style rounding
		ImGui::PopStyleVar(2);
	}

	void drawGizmo(EditorData* data) {
		if (data->selectedEntityID.isValid()) {
			ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
			ImGuizmo::SetRect(data->sceneViewportPos.x, data->sceneViewportPos.y, data->sceneViewportSize.x, data->sceneViewportSize.y);
			ImGuizmo::SetGizmoSizeClipSpace(0.05f * 1080.0f / data->sceneViewportSize.y);

			Entity selectedEntity = getEntityFromID(getActiveScene(data), data->selectedEntityID);

			TransformComponent& selectedTransform = selectedEntity.getComponent<TransformComponent>();
			glm::mat4 worldMatrix = getWorldMatrix(selectedEntity);

			if (data->sceneViewportHovered) {
				if (Input::isKeyPressed(GLFW_KEY_Q)) {
					data->editOperation = ImGuizmo::OPERATION::BOUNDS;
				}
				else if(Input::isKeyPressed(GLFW_KEY_W)) {
					data->editOperation = ImGuizmo::OPERATION::TRANSLATE;
				}
				else if (Input::isKeyPressed(GLFW_KEY_E)) {
					data->editOperation = ImGuizmo::OPERATION::ROTATE;
				}
				else if (Input::isKeyPressed(GLFW_KEY_R)) {
					data->editOperation = ImGuizmo::OPERATION::SCALE;
				}
			}

			glm::mat4 matrixBefore = worldMatrix;

			BoundingBox bounds;
			if (data->editOperation == ImGuizmo::OPERATION::BOUNDS) {
				bounds = BoundingBox{ glm::vec3(-.5f, -.5f, -.5f), glm::vec3(.5f, .5f, .5f) };
				Entity entity = getEntityFromID(getActiveScene(data), data->selectedEntityID);
				if (entity.hasComponent<ModelComponent>()) {
					ModelComponent& modelComponent = entity.getComponent<ModelComponent>();
					if (modelComponent.model) {
						bounds = modelComponent.model->bounds;
					}
				}
			}

			bool shouldSnap = Input::isKeyHeld(GLFW_KEY_LEFT_SHIFT);
			glm::vec3 snap = glm::vec3(0.1f);
			if (data->editOperation == ImGuizmo::OPERATION::ROTATE) {
				snap = glm::vec3(5.0f);
			}

			ImGuizmo::Manipulate(glm::value_ptr(glm::inverse(data->camera.offsetTransform)), glm::value_ptr(data->camera.projection),
				data->editOperation, data->editMode, glm::value_ptr(worldMatrix), nullptr, shouldSnap ? glm::value_ptr(snap) : nullptr, data->editOperation == ImGuizmo::OPERATION::BOUNDS ? (float*)&bounds : nullptr);

			// TODO: Replace when this open issue has been solved: https://github.com/CedricGuillemet/ImGuizmo/issues/201
			if (worldMatrix != matrixBefore) {
				selectedTransform.matrix = toLocalMatrix(worldMatrix, selectedEntity);
			}

			if (Input::isKeyPressed(GLFW_KEY_DELETE)) {
				removeEntity(getActiveScene(data), data->selectedEntityID);
				data->selectedEntityID = UUID::None();
			}
		}
	}

	void drawSceneViewport(EditorData* data) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

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
			ImGui::Image((ImTextureID)data->editorFramebuffer->attachments.at(GL_COLOR_ATTACHMENT0).texture->textureID, data->sceneViewportSize, ImVec2(0, 1), ImVec2(1, 0));
			
			// Create hole for inputs
			ImGui::SetWindowHitTestHole(ImGui::GetCurrentWindow(), data->sceneViewportPos, data->sceneViewportSize);

			// Create clip rect to keep the gizmo inside the framebuffer image
			ImGui::PushClipRect(data->sceneViewportPos, viewportEnd, true);
			drawGizmo(data);
			ImGui::PopClipRect();

			// Picking
			ImVec2 mp = ImGui::GetMousePos();
			data->sceneViewportHovered = mp.x >= data->sceneViewportPos.x && mp.x < viewportEnd.x && mp.y >= data->sceneViewportPos.y && mp.y < viewportEnd.y;

			if (data->sceneViewportHovered && !ImGuizmo::IsUsing() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				bindFramebuffer(*data->editorFramebuffer);
				bindShader(*data->renderer->shader);
				uint32_t objectID;
				glReadBuffer(GL_COLOR_ATTACHMENT1);
				glReadPixels(mp.x - data->sceneViewportPos.x, data->sceneViewportSize.y - (mp.y - data->sceneViewportPos.y), 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &objectID);
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				data->selectedEntityID = objectID;
				data->selectedAsset = nullptr;
				unbindShader();
				unbindFramebuffer();
			}
		}
		ImGui::End();

		ImGui::PopStyleVar();
	}

	void drawGameViewport(EditorData* data) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		if (ImGui::Begin("Game")) {
			// Check if windows size changed
			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size != data->gameViewportSize) {
				data->gameViewportSizeChanged = true;
				data->gameViewportSize = size;
			}

			data->gameViewportPos = ImGui::GetCursorScreenPos();
			const ImVec2 viewportEnd = data->gameViewportPos + data->gameViewportSize;

			if (getActiveScene(data)->mainCameraEntityID.isValid()) {
				// Draw framebuffer
				ImGui::Image(
					(ImTextureID)data->gameFramebuffer->attachments.at(GL_COLOR_ATTACHMENT0).texture->textureID,
					data->gameViewportSize,
					ImVec2(0, 1),
					ImVec2(1, 0)
				);

				// Create hole for inputs
				ImGui::SetWindowHitTestHole(ImGui::GetCurrentWindow(), data->gameViewportPos, data->gameViewportSize);
			}
			else {
				// Missing camera info
				std::string message = "Missing camera";
				ImVec2 msgSize = ImGui::CalcTextSize(message.c_str());
				ImGui::SetCursorPos(ImVec2((size.x - msgSize.x) / 2.0f, (size.y - msgSize.y) / 2.0f));
				ImGui::Text(message.c_str());
			}
		}
		ImGui::End();

		ImGui::PopStyleVar();
	}

	void drawStatusBar(EditorData* data) {
		if (ImGui::Begin("StatusBar")) {
			if (ImGui::Button("Play") && data->playState != PlayModeState::Play) {
				if (!data->scene->mainCameraEntityID.isValid()) {
					ImGui::OpenPopup("No main camera");
				}
				else {
					if (data->playState == PlayModeState::Edit) {
						// Copy scene
						data->runtimeScene = createCopy(data->scene);
						// Set script context active scene
						data->scriptContext->scene = data->runtimeScene;
						// Init entities
						initSceneScriptEntities(data->scriptContext, data->runtimeScene);
						// Start entities
						startScriptEntities(data->scriptContext);
					}

					data->playState = PlayModeState::Play;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop") && data->playState != PlayModeState::Edit) {
				data->playState = PlayModeState::Edit;
				data->scriptContext->scene = data->scene;

				// Reset selected entity ID if it points to a now invalid entity
				if (data->scene->entityMap.find(data->selectedEntityID) == data->scene->entityMap.end()) {
					data->selectedEntityID = UUID::None();
				}

				destroyScene(data->runtimeScene);
				data->runtimeScene = nullptr;
				// TODO: Clean up instances
			}

			// No Camera PopUp
			if (ImGui::BeginPopupModal("No main camera", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
				ImGui::Text("No camera has been selected as main in the current scene.");
				if (ImGui::Button("Close")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}

	void drawEditor(EditorData* data) {
		float menuHeight = drawMenuBar(data);
		drawApplication(data, menuHeight);
		drawHierarchy(getActiveScene(data), data->selectedEntityID);
		drawInspector(data);
		drawAssetViewer(data);
		drawStatusBar(data);
		drawSceneViewport(data);
		drawGameViewport(data);
	}

	Scene* getActiveScene(EditorData* data) {
		if (data->playState == PlayModeState::Edit) {
			return data->scene;
		}
		return data->runtimeScene;
	}

}
