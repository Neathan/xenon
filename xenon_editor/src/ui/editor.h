#pragma once

#include <xenon.h>
#include <imgui.h>
#include <ImGuizmo.h>

#include "orbit_camera.h"

namespace xe {

	struct EditorData {
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

		// SECTION: Viewport (runtime)
		ImVec2 sceneViewportPos;
		ImVec2 sceneViewportSize;
		bool sceneViewportSizeChanged = false;
		
		// SECTION: Editing (runtime)
		UUID selectedEntityID = UUID::None();
		ImGuizmo::MODE editMode = ImGuizmo::MODE::LOCAL;
		ImGuizmo::OPERATION editOperation = ImGuizmo::OPERATION::TRANSLATE;


	};

	EditorData* createEditor();
	void destroyEditor(EditorData* data);

	void drawEditor(EditorData* data);

}
