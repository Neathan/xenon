#include <xenon.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuizmo.h>

#include "orbit_camera.h"
#include "ui_elements.h"
#include "ui/model_inspector.h"

int main() {
	using namespace xe;

	//----------------------------------------
	// SECTION: Application setup
	//----------------------------------------

	XE_SET_LOG_LEVEL(XE_LOG_LEVEL_TRACE);

	Application* application = createApplication("Hello, world!", 1920, 1080);

	// Create PBR renderer and shader
	Shader* shader = loadShader("assets/pbr.vert", "assets/pbr.frag");
	Shader* envShader = loadShader("assets/env.vert", "assets/env.frag");
	Renderer* renderer = createRenderer(shader, envShader);

	// Create framebuffer renderer and shader
	Shader* framebufferShader = loadShader("assets/framebuffer.vert", "assets/framebuffer.frag");
	FramebufferRenderer* framebufferRenderer = createFramebufferRenderer(framebufferShader);

	// Create framebuffer to render to (multisampled)
	Framebuffer* framebuffer = createFramebuffer(1920, 1080, 16);
	framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
	framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::DEPTH, 0));
	framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
	buildFramebuffer(framebuffer);

	// Create framebuffer to blit to (resolved multisample)
	Framebuffer* displayedFramebuffer = createFramebuffer(1920, 1080, 1); // No AA
	displayedFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
	displayedFramebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
	buildFramebuffer(displayedFramebuffer);


	//----------------------------------------
	// SECTION: Test variables
	//----------------------------------------

	// Create orbital camera
	OrbitCamera camera = createOrbitCamera(1920, 1080);

	// Create default scene
	Scene* scene = createScene();

	// Create test entity
	Entity entity = createEntity(scene, "Test Object");
	Model* model = loadModel("assets/MetalRoughSpheres.glb");
	entity.addComponent<ModelComponent>(model);

	// Store test entity transform component
	TransformComponent& transformComponent = entity.getComponent<TransformComponent>();

	// Create light entity
	Entity light = createEntity(scene, "Light");
	Model* lightModel = loadModel("assets/0.1.sphere.glb");
	light.addComponent<PointLight>(glm::vec3(10.0f, 10.0f, 10.0f));
	light.addComponent<ModelComponent>(lightModel);

	// Store light entity transform component
	TransformComponent& lightTransform = light.getComponent<TransformComponent>();

	//----------------------------------------
	// SECTION: ImGui setup
	//----------------------------------------

	ImGuiContext* imguiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(application->window, true);
	ImGui_ImplOpenGL3_Init("#version 460 core");

	//----------------------------------------
	// SECTION: Editing
	//----------------------------------------

	UUID selectedEntityID = UUID::None();

	ImGuizmo::MODE editMode = ImGuizmo::MODE::LOCAL;
	ImGuizmo::OPERATION editOperator = ImGuizmo::OPERATION::UNIVERSAL;

	//----------------------------------------
	// SECTION: Load environment
	//----------------------------------------

	struct EnvironmentAsset {
		std::string path;
		Environment environment;
		bool loaded = false;
	};

	std::vector<EnvironmentAsset> environments;
	//environments.push_back(EnvironmentAsset{ "assets/dikhololo_night_2k.hdr" });
	//environments.push_back(EnvironmentAsset{ "assets/evening_meadow_2k.hdr" });
	//environments.push_back(EnvironmentAsset{ "assets/GCanyon_C_YumaPoint_3k.hdr" });
	//environments.push_back(EnvironmentAsset{ "assets/large_corridor_2k.hdr" });
	//environments.push_back(EnvironmentAsset{ "assets/Road_to_MonumentValley_Ref.hdr" });
	//environments.push_back(EnvironmentAsset{ "assets/studio_small_03_2k.hdr" });

	//environments[0].loaded = true;
	//environments[0].environment = loadIBLCubemap(environments[0].path);

	int currentEnvironment = 0;

	//----------------------------------------
	// SECTION: TESTING AREA
	//----------------------------------------

	TextureParameters textureParams = TextureParameters{ GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
	TextureParameters radianceParams = TextureParameters{ GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
	Environment environment;
	environment.environmentCubemap = loadKTXTexture("assets/output_skybox.ktx", TextureType::IRRADIANCE_CUBEMAP, textureParams);
	environment.irradianceMap = loadKTXTexture("assets/output_iem.ktx", TextureType::IRRADIANCE_CUBEMAP, textureParams);
	environment.radianceMap = loadKTXTexture("assets/output_pmrem.ktx", TextureType::RADIANCE_CUBEMAP, radianceParams);
	environments.push_back(EnvironmentAsset{ "Meadow" , environment, true });

	Environment darkEnv;
	darkEnv.environmentCubemap = loadKTXTexture("assets/dark_skybox.ktx", TextureType::IRRADIANCE_CUBEMAP, textureParams);
	darkEnv.irradianceMap = loadKTXTexture("assets/dark_iem.ktx", TextureType::IRRADIANCE_CUBEMAP, textureParams);
	darkEnv.radianceMap = loadKTXTexture("assets/dark_pmrem.ktx", TextureType::RADIANCE_CUBEMAP, radianceParams);
	environments.push_back(EnvironmentAsset{ "Dark" , darkEnv, true });

	Environment nightEnv;
	nightEnv.environmentCubemap = loadKTXTexture("assets/night_skybox.ktx", TextureType::IRRADIANCE_CUBEMAP, textureParams);
	nightEnv.irradianceMap = loadKTXTexture("assets/night_iem.ktx", TextureType::IRRADIANCE_CUBEMAP, textureParams);
	nightEnv.radianceMap = loadKTXTexture("assets/night_pmrem.ktx", TextureType::RADIANCE_CUBEMAP, radianceParams);
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
		
		// Viewport size change
		if (application->viewportSizeChanged) {
			glViewport(0, 0, application->width, application->height);
			updateFramebufferSize(framebuffer, application->width, application->height);
			updateFramebufferSize(displayedFramebuffer, application->width, application->height);
			updateOrbitCameraProjection(camera, application->width, application->height);
		}

		if (Input::isKeyPressed(GLFW_KEY_F) && selectedEntityID.isValid()) {
			cameraFocus(camera, getWorldMatrix(getEntityFromID(scene, selectedEntityID))[3]);
		}

		if (Input::isKeyPressed(GLFW_KEY_ESCAPE)) {
			selectedEntityID = UUID::None();
		}

		// Camera
		updateOrbitCamera(camera, ts);

		//----------------------------------------
		// SECTION: Render
		//----------------------------------------

		bindFramebuffer(*framebuffer);
		clearFramebuffer(*framebuffer, *renderer->shader);
		renderScene(scene, *renderer, camera, environments[currentEnvironment].environment);
		renderEnvironment(renderer, environments[currentEnvironment].environment, camera, Input::isKeyHeld(GLFW_KEY_SPACE));
		unbindFramebuffer();
		// Blit framebuffer data into displayedFramebuffer (resolve AA data)
		blitFramebuffers(framebuffer, displayedFramebuffer);

		renderFramebufferToScreen(*framebufferRenderer, *displayedFramebuffer);

		//----------------------------------------
		// SECTION: Picking
		//----------------------------------------

		// TODO: Replace with Input system
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !uiWantsMouse) {
			bindFramebuffer(*displayedFramebuffer);
			bindShader(*renderer->shader);
			uint32_t objectID;
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glReadPixels(Input::getMouseX(), application->height - Input::getMouseY(), 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &objectID);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			selectedEntityID = objectID;
			unbindShader();
			unbindFramebuffer();
		}

		//----------------------------------------
		// SECTION: Invoke UI
		//----------------------------------------
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		renderOrbitCameraUI(camera);

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

		// Gizmo UI
		if (selectedEntityID.isValid()) {
			float snap = 0.1f;
			ImGuizmo::SetRect(0, 0, application->width, application->height);
			ImGuizmo::SetGizmoSizeClipSpace(0.05f * 1080.0f / application->height);

			Entity selectedEntity = getEntityFromID(scene, selectedEntityID);

			TransformComponent& selectedTransform = selectedEntity.getComponent<TransformComponent>();
			glm::mat4 worldMatrix = getWorldMatrix(selectedEntity);

			BoundingBox bounds = BoundingBox{glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1)};
			if (selectedEntity.hasComponent<ModelComponent>()) {
				ModelComponent& modelComponent = selectedEntity.getComponent<ModelComponent>();
				if (modelComponent.model) {
					bounds = modelComponent.model->bounds;
				}
			}

			bounds = bounds;
			glm::mat4 matrixBefore = worldMatrix;

			ImGuizmo::Manipulate(glm::value_ptr(camera.inverseTransform), glm::value_ptr(camera.projection),
				editOperator, editMode, glm::value_ptr(worldMatrix), NULL, Input::isKeyHeld(GLFW_KEY_LEFT_SHIFT) ? &snap : NULL, (float*)&bounds);

			// TODO: Replace when this open issue has been solved: https://github.com/CedricGuillemet/ImGuizmo/issues/201
			if(worldMatrix != matrixBefore) {
				selectedTransform.matrix = toLocalMatrix(worldMatrix, selectedEntity);
			}
		}

		// Scene hierarchy
		if (ImGui::Begin("Scene Hierarchy")) {
			drawHierarchy(scene, selectedEntityID);
		}
		ImGui::End();

		// Load model entity
		if (ImGui::Begin("Load model")) {
			static std::string modelPath = "";
			ImGui::InputText("Path", &modelPath);
			if (ImGui::Button("Create entity")) {
				Entity entity = createEntity(scene);
				Model* model = loadModel(modelPath);
				entity.addComponent<ModelComponent>(model);
			}
		}
		ImGui::End();

		// Model inspector
		if (selectedEntityID.isValid()) {
			Entity entity = getEntityFromID(scene, selectedEntityID);
			if (entity.hasComponent<ModelComponent>()) {
				drawModelInspector(*entity.getComponent<ModelComponent>().model);
			}

			if(ImGui::Begin("Model setting")) {
				ImGui::Checkbox("Wireframe", &entity.getComponent<ModelComponent>().wireframe);
			}
			ImGui::End();
		}

		//----------------------------------------
		// SECTION: Render UI
		//----------------------------------------

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		
		//----------------------------------------
		// SECTION: Buffer swap
		//----------------------------------------

		swapBuffers(application);
	}

	//----------------------------------------
	// SECTION: Clean-up
	//----------------------------------------

	ImGui::DestroyContext(imguiContext);

	destroyModel(model);
	destroyModel(lightModel);
	destroyScene(scene);

	destroyFramebufferRenderer(framebufferRenderer);
	destroyShader(framebufferShader);
	destroyFramebuffer(framebuffer);
	destroyRenderer(renderer);
	destroyShader(shader);

	destroyApplication(application);
	return 0;
}