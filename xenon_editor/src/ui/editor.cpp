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

		// Create scene framebuffer to render to (multi sampled)
		editor->editorMultiFB = createFramebuffer(1920, 1080, 16);
		editor->editorMultiFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->editorMultiFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::DEPTH, 0));
		editor->editorMultiFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->editorMultiFB);

		// Create scene framebuffer to blit to (resolved multi sample)
		editor->editorFB = createFramebuffer(1920, 1080, 1); // No AA
		editor->editorFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->editorFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->editorFB);

		// Create game framebuffer to render to (multi sampled)
		editor->gameMultiFB = createFramebuffer(1920, 1080, 16);
		editor->gameMultiFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->gameMultiFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::DEPTH, 0));
		editor->gameMultiFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->gameMultiFB);

		// Create game framebuffer to blit to (resolved multi sample)
		editor->gameFB = createFramebuffer(1920, 1080, 1); // No AA
		editor->gameFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
		editor->gameFB->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
		buildFramebuffer(editor->gameFB);

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
		editor->assetDir = getAsset<Directory>(editor->assetManager, projectFolder, false);

		return editor;
	}

	void destroyEditor(EditorData* data) {
		destroyScriptContext(data->scriptContext);

		destroyScene(data->scene);
		if (data->runtimeScene) {
			destroyScene(data->runtimeScene);
		}

		destroyModel(data->gridModel);

		destroyFramebuffer(data->gameFB);
		destroyFramebuffer(data->gameMultiFB);

		destroyFramebuffer(data->editorFB);
		destroyFramebuffer(data->editorMultiFB);

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
			// TODO(Neathan): Build UI
		}
		ImGui::End();

		// Pop style rounding
		ImGui::PopStyleVar(2);
	}

	void drawGizmo(EditorData* data) {
		ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
		ImGuizmo::SetRect(data->sceneViewPos.x, data->sceneViewPos.y, data->sceneViewSize.x, data->sceneViewSize.y);
		ImGuizmo::SetGizmoSizeClipSpace(0.05f * 1080.0f / data->sceneViewSize.y);

		Entity selectedEntity = getEntityFromID(getActiveScene(data), data->selectedEntityID);

		TransformComponent& selectedTransform = selectedEntity.getComponent<TransformComponent>();
		glm::mat4 worldMatrix = getWorldMatrix(selectedEntity);

		if (data->sceneViewHovered) {
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
		bool editingBounds = data->editOperation == ImGuizmo::OPERATION::BOUNDS;
		glm::vec3 snap = data->editOperation == ImGuizmo::OPERATION::ROTATE ? glm::vec3(5.0f) : glm::vec3(0.1f);

		ImGuizmo::Manipulate(
			glm::value_ptr(glm::inverse(data->camera.offsetTransform)),
			glm::value_ptr(data->camera.projection),
			data->editOperation,
			data->editMode,
			glm::value_ptr(worldMatrix),
			nullptr,
			shouldSnap ? glm::value_ptr(snap) : nullptr,
			editingBounds ? (float*)&bounds : nullptr);

		// TODO(Neathan): Replace when this open issue has been solved: https://github.com/CedricGuillemet/ImGuizmo/issues/201
		if (worldMatrix != matrixBefore) {
			selectedTransform.matrix = toLocalMatrix(worldMatrix, selectedEntity);
		}
	}

	void drawSceneViewport(EditorData* data) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		if (ImGui::Begin("Scene")) {
			// Check if windows size changed
			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size != data->sceneViewSize) {
				data->sceneViewSizeChanged = true;
				data->sceneViewSize = size;
			}

			data->sceneViewPos = ImGui::GetCursorScreenPos();
			const ImVec2 viewportEnd = data->sceneViewPos + data->sceneViewSize;

			// Draw framebuffer
			ImGui::Image(
				(ImTextureID)data->editorFB->attachments.at(GL_COLOR_ATTACHMENT0).texture->textureID,
				data->sceneViewSize,
				ImVec2(0, 1),
				ImVec2(1, 0));
			
			// Create hole for inputs
			ImGui::SetWindowHitTestHole(ImGui::GetCurrentWindow(), data->sceneViewPos, data->sceneViewSize);

			// Draw gizmo
			if (data->selectedEntityID.isValid()) {
				// Create clip rect to keep the gizmo inside the framebuffer image
				ImGui::PushClipRect(data->sceneViewPos, viewportEnd, true);
				drawGizmo(data);
				ImGui::PopClipRect();
			}

			// Picking
			ImVec2 mp = ImGui::GetMousePos();
			data->sceneViewHovered = mp.x >= data->sceneViewPos.x
				&& mp.y >= data->sceneViewPos.y
				&& mp.x < viewportEnd.x
				&& mp.y < viewportEnd.y;

			if (data->sceneViewHovered && !ImGuizmo::IsUsing() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				bindFramebuffer(*data->editorFB);
				bindShader(*data->renderer->shader);

				uint32_t objectID;
				glReadBuffer(GL_COLOR_ATTACHMENT1);
				glReadPixels(
					mp.x - data->sceneViewPos.x,
					data->sceneViewSize.y - (mp.y - data->sceneViewPos.y),
					1, 1,
					GL_RED_INTEGER,
					GL_UNSIGNED_INT,
					&objectID);
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
			Scene* activeScene = getActiveScene(data);
			
			// Check if windows size changed
			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size != data->gameViewSize) {
				data->gameViewSizeChanged = true;
				data->gameViewSize = size;
			}

			data->gameViewPos = ImGui::GetCursorScreenPos();
			const ImVec2 viewportEnd = data->gameViewPos + data->gameViewSize;

			if (activeScene->mainCameraEntityID.isValid()) {
				// Draw framebuffer
				ImGui::Image(
					(ImTextureID)data->gameFB->attachments.at(GL_COLOR_ATTACHMENT0).texture->textureID,
					data->gameViewSize,
					ImVec2(0, 1),
					ImVec2(1, 0)
				);

				// Create hole for inputs
				ImGui::SetWindowHitTestHole(ImGui::GetCurrentWindow(), data->gameViewPos, data->gameViewSize);
			}
			else {
				// Missing camera info
				std::string message = "Missing camera";
				ImVec2 msgSize = ImGui::CalcTextSize(message.c_str());
				ImGui::SetCursorPos(ImVec2((size.x - msgSize.x) / 2.0f, (size.y - msgSize.y) / 2.0f));
				ImGui::Text(message.c_str());
			}

			// Remove entity on delete
			if (Input::isKeyPressed(GLFW_KEY_DELETE)) {
				removeEntity(activeScene, data->selectedEntityID);
				data->selectedEntityID = UUID::None();
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
			ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
			if (ImGui::BeginPopupModal("No main camera", nullptr, flags)) {
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
		drawHierarchy(data);
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
