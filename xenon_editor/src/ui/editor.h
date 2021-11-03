#pragma once

#include <xenon.h>
#include <imgui.h>
#include <ImGuizmo.h>

#include "orbit_camera.h"

namespace xe {

	enum class PlayModeState {
		Edit,
		Play,
		Pause
	};

	struct EditorData {
		// SECTION: Assets (initialized)
		AssetManager* assetManager;

		// SECTION: Rendering (initialized)
		Shader* pbrShader = nullptr;
		Shader* envShader = nullptr;
		Shader* gridShader = nullptr;
		Renderer* renderer = nullptr;

		Framebuffer* framebuffer = nullptr;
		Framebuffer* displayedFramebuffer = nullptr;

		Model* gridModel = nullptr;

		// SECTION: Viewport (initialized)
		OrbitCamera camera;
		Scene* scene = nullptr; // TODO: Make runtime determined
		Scene* runtimeScene = nullptr;
		
		// SECTION: Scripting (initialized)
		ScriptContext* scriptContext = nullptr;


		// SECTION: Asset viewer (initialized)
		Directory* assetViewerDirectory = nullptr;
		Asset* selectedAsset = nullptr;

		// SECTION: Viewport (runtime)
		ImVec2 sceneViewportPos;
		ImVec2 sceneViewportSize;
		bool sceneViewportSizeChanged = false;
		bool sceneViewportHovered = false;
		
		// SECTION: Editing (runtime)
		UUID selectedEntityID = UUID::None();
		ImGuizmo::MODE editMode = ImGuizmo::MODE::LOCAL;
		ImGuizmo::OPERATION editOperation = ImGuizmo::OPERATION::TRANSLATE;

		// SECTION: Runtime (runtime)
		PlayModeState playState = PlayModeState::Edit;

	};

	EditorData* createEditor(const std::string& projectFolder);
	void destroyEditor(EditorData* data);

	void drawEditor(EditorData* data);

	Scene* getActiveScene(EditorData* data);

}
