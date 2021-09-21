#include <xenon.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <ImGuizmo.h>

#include "orbit_camera.h"
#include "ui_elements.h"

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

	// Create framebuffer to render to
	Framebuffer* framebuffer = createFramebuffer(1920, 1080);
	framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::COLOR, 0));
	framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::DEPTH, 0));
	framebuffer->attachments.insert(createDefaultFramebufferAttachment(DefaultAttachmentType::INTEGER, 1));
	buildFramebuffer(framebuffer);

	//----------------------------------------
	// SECTION: Test variables
	//----------------------------------------

	// Create orbital camera
	OrbitCamera camera = createOrbitCamera(1920, 1080);

	// Create default scene
	Scene* scene = createScene();

	// Create test entity
	Entity entity = createEntity(scene, "Test Object");
	Model* model = loadModel("assets/WaterBottle.glb");
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
	environments.push_back(EnvironmentAsset{ "assets/dikhololo_night_2k.hdr" });
	environments.push_back(EnvironmentAsset{ "assets/evening_meadow_2k.hdr" });
	environments.push_back(EnvironmentAsset{ "assets/GCanyon_C_YumaPoint_3k.hdr" });
	environments.push_back(EnvironmentAsset{ "assets/large_corridor_2k.hdr" });
	environments.push_back(EnvironmentAsset{ "assets/Road_to_MonumentValley_Ref.hdr" });
	environments.push_back(EnvironmentAsset{ "assets/studio_small_03_2k.hdr" });

	environments[0].loaded = true;
	environments[0].environment = loadIBLCubemap(environments[0].path);

	int currentEnvironment = 0;

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
			updateOrbitCameraProjection(camera, application->width, application->height);
		}

		// Camera
		updateOrbitCamera(camera);

		//----------------------------------------
		// SECTION: Render
		//----------------------------------------

		bindFramebuffer(*framebuffer);
		clearFramebuffer(*framebuffer, *renderer->shader);
		renderScene(scene, *renderer, camera, environments[currentEnvironment].environment);
		renderEnvironment(renderer, environments[currentEnvironment].environment, camera, Input::isKeyHeld(GLFW_KEY_SPACE));
		unbindFramebuffer();

		renderFramebufferToScreen(*framebufferRenderer, *framebuffer);

		//----------------------------------------
		// SECTION: Picking
		//----------------------------------------

		// TODO: Replace with Input system
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !uiWantsMouse) {
			bindFramebuffer(*framebuffer);
			bindShader(*renderer->shader);
			uint32_t objectID;
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glReadPixels(Input::getMouseX(), application->height - Input::getMouseY(), 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &objectID);
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

			Entity selectedEntity = getEntityFromID(scene, selectedEntityID);

			TransformComponent& selectedTransform = selectedEntity.getComponent<TransformComponent>();
			glm::mat4 worldMatrix = getWorldMatrix(selectedEntity);

			if (ImGuizmo::Manipulate(glm::value_ptr(camera.inverseTransform), glm::value_ptr(camera.projection),
				editOperator, editMode, glm::value_ptr(worldMatrix), NULL, Input::isKeyHeld(GLFW_KEY_LEFT_SHIFT) ? &snap : NULL)) {

				selectedTransform.matrix = toLocalMatrix(worldMatrix, selectedEntity);
			}
		}

		// Scene hierarchy
		if (ImGui::Begin("Scene Hierarchy")) {
			drawHierarchy(scene, selectedEntityID);
		}
		ImGui::End();

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