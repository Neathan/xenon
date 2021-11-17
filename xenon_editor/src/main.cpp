#include <xenon.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <misc/cpp/imgui_stdlib.h>

#include "ui/theme.h"
#include "ui/editor.h"

int main() {
	using namespace xe;

	//----------------------------------------
	// SECTION: Application setup
	//----------------------------------------

	XE_SET_LOG_LEVEL(XE_LOG_LEVEL_TRACE);

	Application* application = createApplication("Hello, world!", 1920, 1080);
	maximizeApplication(application);


	//----------------------------------------
	// SECTION: ImGui setup
	//----------------------------------------

	ImGuiContext* imguiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	
	ImGui::StyleColorsDark();
	setTheme();

	// Load font
	ImFontConfig fontConfig;
	fontConfig.OversampleH = 2;
	fontConfig.OversampleV = 2;
	fontConfig.GlyphExtraSpacing.x = 0.15f;

	io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 16.0f, &fontConfig);

	ImGui_ImplGlfw_InitForOpenGL(application->window, true);
	ImGui_ImplOpenGL3_Init("#version 460 core");


	//----------------------------------------
	// SECTION: Editor
	//----------------------------------------

	EditorData* editor = createEditor("assets/");


	//----------------------------------------
	// SECTION: Scripting
	//----------------------------------------

	loadScriptAssembly(editor->scriptContext, "Test.dll");


	//----------------------------------------
	// SECTION: Load environment
	//----------------------------------------

	struct EnvironmentAsset {
		std::string path;
		Environment env;

		bool loaded = false;
	};

	std::vector<EnvironmentAsset> envAssets;
	int currentEnv = 0;


	//----------------------------------------
	// SECTION: TESTING AREA
	//----------------------------------------

	TextureParameters textureParams = TextureParameters{
		GL_LINEAR, GL_LINEAR,
		GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE
	};
	TextureParameters radianceParams = TextureParameters{
		GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
		GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE
	};
	
	Environment environment;
	environment.environmentCubemap = loadTexture("assets/environments/output_skybox.ktx", GL_RGB16F, textureParams);
	environment.irradianceMap = loadTexture("assets/environments/output_iem.ktx", GL_RGB16F, textureParams);
	environment.radianceMap = loadTexture("assets/environments/output_pmrem.ktx", GL_RGB16F, radianceParams);
	envAssets.push_back(EnvironmentAsset{ "Meadow" , environment, true });

	Environment darkEnv;
	darkEnv.environmentCubemap = loadTexture("assets/environments/dark_skybox.ktx", GL_RGB16F, textureParams);
	darkEnv.irradianceMap = loadTexture("assets/environments/dark_iem.ktx", GL_RGB16F, textureParams);
	darkEnv.radianceMap = loadTexture("assets/environments/dark_pmrem.ktx", GL_RGB16F, radianceParams);
	envAssets.push_back(EnvironmentAsset{ "Dark" , darkEnv, true });

	Environment nightEnv;
	nightEnv.environmentCubemap = loadTexture("assets/environments/night_skybox.ktx", GL_RGB16F, textureParams);
	nightEnv.irradianceMap = loadTexture("assets/environments/night_iem.ktx", GL_RGB16F, textureParams);
	nightEnv.radianceMap = loadTexture("assets/environments/night_pmrem.ktx", GL_RGB16F, radianceParams);
	envAssets.push_back(EnvironmentAsset{ "Night" , darkEnv, true });


	//----------------------------------------
	// SECTION: Program loop
	//----------------------------------------

	while (!shouldApplicationClose(application)) {
		Timestep ts = updateApplication(application);

		Scene* activeScene = getActiveScene(editor);

		//----------------------------------------
		// SECTION: UI state
		//----------------------------------------

		bool uiWantsMouse = io.WantCaptureMouse || ImGuizmo::IsUsing();
		bool uiWantsKeyboard = io.WantCaptureKeyboard;


		//----------------------------------------
		// SECTION: Update
		//----------------------------------------

		if (Input::isKeyPressed(GLFW_KEY_F) && editor->selectedEntityID.isValid()) {
			Entity selected = getEntityFromID(activeScene, editor->selectedEntityID);
			cameraFocus(editor->camera, getWorldMatrix(selected)[3]);
		}

		if (Input::isKeyPressed(GLFW_KEY_ESCAPE)) {
			editor->selectedEntityID = UUID::None();
			editor->selectedAsset = nullptr;
		}

		// Camera
		if (editor->sceneViewHovered) {
			updateOrbitCamera(editor->camera, ts);
		}

		// Scripts & Animations
		updateSceneModels(activeScene, editor->playState == PlayModeState::Play, ts.deltaTime);
		if (editor->playState == PlayModeState::Play) {
			updateScriptEntities(editor->scriptContext, ts.deltaTime);
		}


		//----------------------------------------
		// SECTION: Render
		//----------------------------------------

		// Render game view
		bool isGameShown = activeScene->mainCameraEntityID.isValid()
			&& editor->gameViewSize.x != -1
			&& editor->gameViewSize.y != -1;

		if (isGameShown) {
			glViewport(0, 0, editor->gameViewSize.x, editor->gameViewSize.y);

			bindFramebuffer(*editor->gameMultiFB);
			clearFramebuffer(*editor->gameMultiFB, *editor->renderer->shader);
			renderScene(activeScene, *editor->renderer, envAssets[currentEnv].env);
			unbindFramebuffer();

			// Blit framebuffer data into displayedFramebuffer (resolve AA data)
			blitFramebuffers(editor->gameMultiFB, editor->gameFB);
		}

		// Render editor view
		bool isEditorShown = editor->sceneViewSize.x != -1 && editor->sceneViewSize.y != -1;

		if(isEditorShown) {
			glViewport(0, 0, editor->sceneViewSize.x, editor->sceneViewSize.y);

			bindFramebuffer(*editor->editorMultiFB);
			clearFramebuffer(*editor->editorMultiFB, *editor->renderer->shader);
			renderSceneCustomCamera(activeScene, *editor->renderer, envAssets[currentEnv].env, editor->camera);

			// TODO(Neathan): Move to framebuffer function
			// Disable rendering to objectID attachment
			glNamedFramebufferDrawBuffer(editor->editorMultiFB->frambufferID, GL_COLOR_ATTACHMENT0);

			//renderEnvironment(editorData->renderer, environments[currentEnvironment].environment, editorData->camera);
			renderGrid(editor->gridShader, editor->gridModel, editor->camera);

			// TODO(Neathan): Move to framebuffer function
			// Re-enable rendering to objectID attachment
			glNamedFramebufferDrawBuffers(
				editor->editorMultiFB->frambufferID,
				editor->editorMultiFB->colorBuffers.size(),
				editor->editorMultiFB->colorBuffers.data()
			);
			unbindFramebuffer();
			
			// Blit framebuffer data into displayedFramebuffer (resolve AA data)
			blitFramebuffers(editor->editorMultiFB, editor->editorFB);
		}


		//----------------------------------------
		// SECTION: Create UI
		//----------------------------------------
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		drawEditor(editor);

		ImGui::ShowDemoWindow();


		//----------------------------------------
		// SECTION: Render UI
		//----------------------------------------

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		

		//----------------------------------------
		// SECTION: Update framebuffer size and swap buffers
		//----------------------------------------
		
		if (editor->sceneViewSizeChanged) {
			editor->sceneViewSizeChanged = false;
			if (editor->sceneViewSize.x > 0 && editor->sceneViewSize.y > 0) {
				updateFramebufferSize(editor->editorMultiFB, editor->sceneViewSize.x, editor->sceneViewSize.y);
				updateFramebufferSize(editor->editorFB, editor->sceneViewSize.x, editor->sceneViewSize.y);
				updateOrbitCameraProjection(editor->camera, editor->sceneViewSize.x, editor->sceneViewSize.y);
			}
		}

		if (editor->gameViewSizeChanged) {
			editor->gameViewSizeChanged = false;
			if (editor->gameViewSize.x > 0 && editor->gameViewSize.y > 0) {
				updateFramebufferSize(editor->gameMultiFB, editor->gameViewSize.x, editor->gameViewSize.y);
				updateFramebufferSize(editor->gameFB, editor->gameViewSize.x, editor->gameViewSize.y);

				// Update main camera projection
				if (activeScene->mainCameraEntityID.isValid()) {
					Entity cameraEntity = getEntityFromID(activeScene, activeScene->mainCameraEntityID);
					CameraComponent& camera = cameraEntity.getComponent<CameraComponent>();

					camera.width = editor->gameViewSize.x;
					camera.height = editor->gameViewSize.y;
					updateCameraProjection(camera);
				}
			}
		}

		swapBuffers(application);
	}


	//----------------------------------------
	// SECTION: Clean-up
	//----------------------------------------

	ImGui::DestroyContext(imguiContext);
	destroyEditor(editor);
	destroyApplication(application);
	return 0;
}