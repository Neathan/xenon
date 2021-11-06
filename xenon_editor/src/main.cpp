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
	ImFontConfig fontConfig; fontConfig.OversampleH = 2; fontConfig.OversampleV = 2, fontConfig.GlyphExtraSpacing.x = 0.15f;
	io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 16.0f, &fontConfig);

	ImGui_ImplGlfw_InitForOpenGL(application->window, true);
	ImGui_ImplOpenGL3_Init("#version 460 core");

	//----------------------------------------
	// SECTION: Editor
	//----------------------------------------

	EditorData* editorData = createEditor("assets/");

	//----------------------------------------
	// SECTION: Scripting
	//----------------------------------------

	loadScriptAssembly(editorData->scriptContext, "Test.dll");

	//----------------------------------------
	// SECTION: Load environment
	//----------------------------------------

	struct EnvironmentAsset {
		std::string path;
		Environment environment;
		bool loaded = false;
	};

	std::vector<EnvironmentAsset> environments;
	int currentEnvironment = 0;

	//----------------------------------------
	// SECTION: TESTING AREA
	//----------------------------------------

	TextureParameters textureParams = TextureParameters{ GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
	TextureParameters radianceParams = TextureParameters{ GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
	Environment environment;
	environment.environmentCubemap = loadKTXTexture("assets/environments/output_skybox.ktx", textureParams);
	environment.irradianceMap = loadKTXTexture("assets/environments/output_iem.ktx", textureParams);
	environment.radianceMap = loadKTXTexture("assets/environments/output_pmrem.ktx", radianceParams);
	environments.push_back(EnvironmentAsset{ "Meadow" , environment, true });

	Environment darkEnv;
	darkEnv.environmentCubemap = loadKTXTexture("assets/environments/dark_skybox.ktx", textureParams);
	darkEnv.irradianceMap = loadKTXTexture("assets/environments/dark_iem.ktx", textureParams);
	darkEnv.radianceMap = loadKTXTexture("assets/environments/dark_pmrem.ktx", radianceParams);
	environments.push_back(EnvironmentAsset{ "Dark" , darkEnv, true });

	Environment nightEnv;
	nightEnv.environmentCubemap = loadKTXTexture("assets/environments/night_skybox.ktx", textureParams);
	nightEnv.irradianceMap = loadKTXTexture("assets/environments/night_iem.ktx", textureParams);
	nightEnv.radianceMap = loadKTXTexture("assets/environments/night_pmrem.ktx", radianceParams);
	environments.push_back(EnvironmentAsset{ "Night" , darkEnv, true });



	//----------------------------------------
	// SECTION: Program loop
	//----------------------------------------

	while (!shouldApplicationClose(application)) {
		Timestep ts = updateApplication(application);

		bool uiWantsMouse = io.WantCaptureMouse || ImGuizmo::IsUsing();
		bool uiWantsKeyboard = io.WantCaptureKeyboard;

		//----------------------------------------
		// SECTION: Update
		//----------------------------------------

		if (Input::isKeyPressed(GLFW_KEY_F) && editorData->selectedEntityID.isValid()) {
			cameraFocus(editorData->camera, getWorldMatrix(getEntityFromID(getActiveScene(editorData), editorData->selectedEntityID))[3]);
		}

		if (Input::isKeyPressed(GLFW_KEY_ESCAPE)) {
			editorData->selectedEntityID = UUID::None();
			editorData->selectedAsset = nullptr;
		}

		// Camera
		if (editorData->sceneViewportHovered) {
			updateOrbitCamera(editorData->camera, ts);
		}

		// Scripts & Animations
		updateSceneModels(getActiveScene(editorData), ts.deltaTime);
		if (editorData->playState == PlayModeState::Play) {
			updateScriptEntities(editorData->scriptContext, ts.deltaTime);
		}

		//----------------------------------------
		// SECTION: Render
		//----------------------------------------

		bindFramebuffer(*editorData->framebuffer);
		clearFramebuffer(*editorData->framebuffer, *editorData->renderer->shader);
		renderScene(getActiveScene(editorData), *editorData->renderer, editorData->camera, environments[currentEnvironment].environment);
		// TODO: Make this nicer
		// Disable rendering to objectID attachment
		glNamedFramebufferDrawBuffer(editorData->framebuffer->frambufferID, GL_COLOR_ATTACHMENT0);
		//renderEnvironment(editorData->renderer, environments[currentEnvironment].environment, editorData->camera);
		renderGrid(editorData->gridShader, editorData->gridModel, editorData->camera);
		// Re-enable rendering to objectID attachment
		glNamedFramebufferDrawBuffers(editorData->framebuffer->frambufferID, editorData->framebuffer->colorBuffers.size(), editorData->framebuffer->colorBuffers.data());
		unbindFramebuffer();
		// Blit framebuffer data into displayedFramebuffer (resolve AA data)
		blitFramebuffers(editorData->framebuffer, editorData->displayedFramebuffer);
		

		//----------------------------------------
		// SECTION: Invoke UI
		//----------------------------------------
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		drawEditor(editorData);

		ImGui::ShowDemoWindow();

		//----------------------------------------
		// SECTION: Test components (components without a proper home yet)
		//----------------------------------------

		/*
		// Environment selector
		if (ImGui::Begin("Environment")) {
			for (size_t i = 0; i < environments.size(); ++i) {
				if (ImGui::Button(environments[i].path.c_str())) {
					currentEnvironment = i;
					if (!environments[i].loaded) {
						environments[i].environment = loadIBLCubemap(environments[i].path);
						environments[i].loaded = true;
					}
				}
			}
		}
		ImGui::End();
		*/

		//----------------------------------------
		// SECTION: Render UI
		//----------------------------------------

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		//----------------------------------------
		// SECTION: Update framebuffer size and swap buffers
		//----------------------------------------
		
		if (editorData->sceneViewportSizeChanged) {
			editorData->sceneViewportSizeChanged = false;
			if (editorData->sceneViewportSize.x > 0 && editorData->sceneViewportSize.y > 0) {
				glViewport(0, 0, editorData->sceneViewportSize.x, editorData->sceneViewportSize.y);
				updateFramebufferSize(editorData->framebuffer, editorData->sceneViewportSize.x, editorData->sceneViewportSize.y);
				updateFramebufferSize(editorData->displayedFramebuffer, editorData->sceneViewportSize.x, editorData->sceneViewportSize.y);
				updateOrbitCameraProjection(editorData->camera, editorData->sceneViewportSize.x, editorData->sceneViewportSize.y);
			}
		}

		swapBuffers(application);
	}

	//----------------------------------------
	// SECTION: Clean-up
	//----------------------------------------

	ImGui::DestroyContext(imguiContext);
	destroyEditor(editorData);
	destroyApplication(application);
	return 0;
}