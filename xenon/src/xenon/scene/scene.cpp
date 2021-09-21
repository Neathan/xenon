#include "scene.h"

#include "xenon/core/assert.h"
#include "xenon/graphics/environment.h"

namespace xe {

	//----------------------------------------
	// SECTION: Entity functions
	//----------------------------------------

	UUID getEntityID(Entity entity) {
		return entity.getComponent<IdentityComponent>().uuid;
	}


	//----------------------------------------
	// SECTION: Scene functions
	//----------------------------------------

	Scene* createScene() {
		return new Scene();
	}

	void destroyScene(Scene* scene) {
		delete scene;
	}

	Entity createEntity(Scene* scene, const std::string& name) {
		Entity entity = { scene->registry.create(), scene };
		entity.addComponent<TransformComponent>();
		IdentityComponent& identity = entity.addComponent<IdentityComponent>();
		identity.name = name;
		scene->entityMap[identity.uuid] = entity;
		return entity;
	}

	Entity createEntityWithID(Scene* scene, const std::string& name, const UUID& id) {
		Entity entity = { scene->registry.create(), scene };
		entity.addComponent<TransformComponent>();
		IdentityComponent& identity = entity.addComponent<IdentityComponent>();
		identity.uuid = id;
		identity.name = name;
		scene->entityMap[identity.uuid] = entity;
		return entity;
	}

	void removeEntity(Scene* scene, UUID id) {
		XE_ASSERT(scene->entityMap.find(id) != scene->entityMap.end());
		/* TODO: Parental hierarchy
		auto children = scene->entityMap.at(id).getComponent<TransformComponent>().children;

		for (auto child : children) {
			scene->entityMap.at(child).setParent(UUID::None());
		}
		scene->entityMap.at(id).setParent(UUID::None());
		*/
		scene->registry.destroy(scene->entityMap.at(id).handle);
		scene->entityMap.erase(id);
	}

	Entity getEntityFromID(Scene* scene, UUID id) {
		return scene->entityMap[id];
	}


	glm::mat4 getWorldMatrix(Entity entity) {
		TransformComponent& transformComponent = entity.getComponent<TransformComponent>();
		glm::mat4 localMatrix = transformComponent.matrix;
		UUID parentID = transformComponent.parent;
		if (parentID.isValid()) {
			return getWorldMatrix(getEntityFromID(entity.scene, parentID)) * localMatrix;
		}
		return localMatrix;
	}

	glm::mat4 toLocalMatrix(glm::mat4 matrix, Entity entity) {
		TransformComponent& transformComponent = entity.getComponent<TransformComponent>();
		if (!transformComponent.parent.isValid()) {
			return matrix;
		}
		return glm::inverse(getWorldMatrix(getEntityFromID(entity.scene, transformComponent.parent))) * matrix;
	}

	void renderScene(Scene* scene, const Renderer& renderer, const Camera& camera, const Environment& environment) {
		// Load lights
		auto lightView = scene->registry.view<PointLight, IdentityComponent, TransformComponent>();
		int index = 0;
		bindShader(*renderer.shader);
		for (const auto [entity, pointLight, identity, transform] : lightView.each()) {
			loadLight(*renderer.shader, getWorldMatrix(getEntityFromID(scene, identity.uuid))[3], pointLight, index++);
		}
		loadInt(*renderer.shader, "pointLightsUsed", index);
		
		// Load environment ibl
		glActiveTexture(GL_TEXTURE0 + (int)TextureType::IBL_CUBEMAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, environment.iblCubemap->textureID);
		glActiveTexture(GL_TEXTURE0);
		unbindShader();

		// Render models
		auto modelView = scene->registry.view<ModelComponent, IdentityComponent>();
		for (auto [entity, modelComponent, identityComponent] : modelView.each()) {
			if (modelComponent.model) {
				setObjectID(renderer, identityComponent.uuid);
				renderModel(renderer, *modelComponent.model, getWorldMatrix({ entity, scene }), camera);
			}
		}
	}

}
