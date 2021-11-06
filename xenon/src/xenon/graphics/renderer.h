#pragma once

#include "xenon/graphics/shader.h"
#include "xenon/graphics/model.h"
#include "xenon/graphics/material.h"
#include "xenon/graphics/camera.h"
#include "xenon/graphics/framebuffer.h"
#include "xenon/graphics/environment.h"

#include "xenon/core/uuid.h"

namespace xe {

	//----------------------------------------
	// SECTION: Renderer
	//----------------------------------------

	struct Renderer {
		Shader* shader;
		Shader* envShader;
		Texture* brdfLUT;
		Model* envCubeModel = nullptr;
	};

	Renderer* createRenderer(Shader* shader, Shader* envShader);
	void destroyRenderer(Renderer* renderer);


	//----------------------------------------
	// SECTION: Renderer functions
	//----------------------------------------

	void setObjectID(const Renderer& renderer, UUID id);
	void renderModel(const Renderer& renderer, const ModelComponent& modelComponent, const glm::mat4& transform, const Camera& camera, bool ignoreMaterials = false);
	void renderEnvironment(Renderer* renderer, const Environment& environment, const Camera& camera);

	void renderGrid(Shader* shader, Model* model, const Camera& camera);

	//----------------------------------------
	// SECTION: Framebuffer renderer
	//----------------------------------------

	struct FramebufferRenderer {
		Shader* shader;
		Model* planeModel;
	};

	FramebufferRenderer* createFramebufferRenderer(Shader* shader);
	void destroyFramebufferRenderer(FramebufferRenderer* renderer);


	//----------------------------------------
	// SECTION: Framebuffer renderer functions
	//----------------------------------------

	void renderFramebufferToScreen(const FramebufferRenderer& renderer, const Framebuffer& framebuffer);
}
