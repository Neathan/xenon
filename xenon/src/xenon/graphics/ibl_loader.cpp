#include "ibl_loader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "xenon/graphics/framebuffer.h"
#include "xenon/graphics/shader.h"

#include "xenon/graphics/primitives.h"

namespace xe {

	Environment loadIBLCubemap(const std::string& path, int resolution, int iblResolution) {
		Environment environment;

		TextureParameters textureParams = TextureParameters{ GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
		std::shared_ptr<Texture> iblTexture = loadTexture(path, TextureType::IBL_SOURCE, TextureFormat::RGB_FLOAT, textureParams);
		environment.irradianceMap = createEmptyCubemapTexture(iblResolution, TextureType::IRRADIANCE_CUBEMAP, TextureFormat::RGB_FLOAT, textureParams);
		environment.environmentCubemap = createEmptyCubemapTexture(resolution, TextureType::IRRADIANCE_CUBEMAP, TextureFormat::RGB_FLOAT, textureParams);
		
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] = {
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		// Create framebuffer to host cubemap texture
		Framebuffer* framebuffer = createFramebuffer(resolution, resolution);

		// Load shaders
		Shader* equiShader = loadShader("assets/ibl_cubemap.vert", "assets/equirectangular.frag");
		Shader* iblShader = loadShader("assets/ibl_cubemap.vert", "assets/ibl.frag");

		// Load/Generate cube model
		Model* model = generateCubeModel(glm::vec3(1.0f));


		// STEP 1:
		// Convert ibl source texture to cubemap

		// Bind framebuffer and shader
		bindFramebuffer(*framebuffer);
		bindShader(*equiShader);
		
		loadMat4(*equiShader, "projection", captureProjection);

		// Query old viewport and set new viewport size to given resolution
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		glViewport(0, 0, resolution, resolution);
		
		// Load IBL Source texture
		glBindTextureUnit(0, iblTexture->textureID);
		
		// Render
		glDisable(GL_CULL_FACE);
		for (int face = 0; face < 6; ++face) {
			loadMat4(*equiShader, "view", captureViews[face]);
			glNamedFramebufferTextureLayer(framebuffer->frambufferID, GL_COLOR_ATTACHMENT0, environment.environmentCubemap->textureID, 0, face);
			glClear(GL_COLOR_BUFFER_BIT);
			
			// Render cube
			const Primitive& primitive = model->primitives[0];
			glBindVertexArray(primitive.vao);
			glDrawArrays(primitive.mode, 0, primitive.count);
		}
		glBindVertexArray(0);
		glEnable(GL_CULL_FACE);
		glBindTextureUnit(0, 0);
		unbindShader();
		unbindFramebuffer();


		// STEP 2:
		// Create irradiance map from cubemap
		bindFramebuffer(*framebuffer);
		bindShader(*iblShader);

		loadMat4(*iblShader, "projection", captureProjection);
		glViewport(0, 0, iblResolution, iblResolution);

		// Load cubemap
		glBindTextureUnit(0, environment.environmentCubemap->textureID);

		glDisable(GL_CULL_FACE);
		for (int face = 0; face < 6; ++face) {
			loadMat4(*iblShader, "view", captureViews[face]);
			glNamedFramebufferTextureLayer(framebuffer->frambufferID, GL_COLOR_ATTACHMENT0, environment.irradianceMap->textureID, 0, face);
			glClear(GL_COLOR_BUFFER_BIT);

			// Render cube
			const Primitive& primitive = model->primitives[0];
			glBindVertexArray(primitive.vao);
			glDrawArrays(primitive.mode, 0, primitive.count);
		}
		glBindVertexArray(0);
		glEnable(GL_CULL_FACE);
		glBindTextureUnit(0, 0);
		unbindShader();
		unbindFramebuffer();


		// Clean up
		destroyModel(model);
		destroyShader(equiShader);
		destroyShader(iblShader);
		destroyFramebuffer(framebuffer);

		// Restore viewport
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		return environment;
	}

}
