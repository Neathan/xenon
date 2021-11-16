#include "scene.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "xenon/core/assert.h"
#include "xenon/graphics/environment.h"

#include "xenon/scripting/script.h"


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

	static void onCameraComponentConstruct(entt::registry& registry, entt::entity entityHandle) {
		updateCameraProjection(registry.get<CameraComponent>(entityHandle));
	}

	Scene* createScene() {
		Scene* scene = new Scene();
		scene->registry.emplace<SceneComponent>(scene->registry.create(), scene);
		scene->registry.on_construct<SceneComponent>().connect<&onCameraComponentConstruct>();
		return scene;
	}

	void destroyScene(Scene* scene) {
		if (getActiveContext()) {
			destroyScriptScene(getActiveContext(), scene);
		}
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

		for (auto& [uuid, entity] : scene->entityMap) {
			TransformComponent& transform = entity.getComponent<TransformComponent>();
			if (uuid != id && transform.parent == id) {
				transform.matrix = getWorldMatrix(entity);;
				transform.parent = UUID::None();
			}
		}
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

	glm::mat4 getParentWorldMatrix(Entity entity) {
		UUID parentID = entity.getComponent<TransformComponent>().parent;
		if (parentID.isValid()) {
			return getWorldMatrix(getEntityFromID(entity.scene, parentID));
		}
		return glm::mat4(1.0f);
	}

	glm::mat4 toLocalMatrix(glm::mat4 matrix, Entity entity) {
		TransformComponent& transformComponent = entity.getComponent<TransformComponent>();
		if (!transformComponent.parent.isValid()) {
			return matrix;
		}
		return glm::inverse(getWorldMatrix(getEntityFromID(entity.scene, transformComponent.parent))) * matrix;
	}

	void renderScene(Scene* scene, const Renderer& renderer, const Environment& environment) {
		Entity cameraEntity = getEntityFromID(scene, scene->mainCameraEntityID);
		Camera& camera = cameraEntity.getComponent<CameraComponent>().camera;
		glm::mat4 cameraTransform = getWorldMatrix(cameraEntity);
		renderSceneCustomCamera(scene, renderer, environment, camera, cameraTransform);
	}

	void renderSceneCustomCamera(Scene* scene, const Renderer& renderer, const Environment& environment, const Camera& camera, const glm::mat4& cameraTransform) {
		// Load lights
		auto lightView = scene->registry.view<PointLightComponent, IdentityComponent, TransformComponent>();
		int index = 0;
		bindShader(*renderer.shader);
		for (const auto [entity, pointLight, identity, transform] : lightView.each()) {
			loadLight(*renderer.shader, getWorldMatrix(getEntityFromID(scene, identity.uuid))[3], pointLight, index++);
		}
		loadInt(*renderer.shader, "pointLightsUsed", index);
		
		// Load environment and BRDF
		glBindTextureUnit(5, environment.irradianceMap->textureID);
		glBindTextureUnit(6, environment.radianceMap->textureID);
		glBindTextureUnit(7, renderer.brdfLUT->textureID);

		// Load camera
		loadCamera(*renderer.shader, camera, cameraTransform);

		unbindShader();

		// Render models
		auto modelView = scene->registry.view<ModelComponent, IdentityComponent>();
		for (auto [entity, modelComponent, identityComponent] : modelView.each()) {
			if (modelComponent.model) {
				if(modelComponent.wireframe) glPolygonMode(GL_FRONT, GL_LINE);
				setObjectID(renderer, identityComponent.uuid);
				renderModel(renderer, modelComponent, getWorldMatrix({ entity, scene }));
				if (modelComponent.wireframe) glPolygonMode(GL_FRONT, GL_FILL);
			}
		}
	}

	void updateSceneModels(Scene* scene, bool isPlaying, float delta) {
		auto modelView = scene->registry.view<ModelComponent>();
		for (auto [entity, modelComponent] : modelView.each()) {
			if (modelComponent.model) {
				updateAnimation(modelComponent, isPlaying, delta);
				updateInstanceTransformation(modelComponent, getParentWorldMatrix({ entity, scene }));
			}
		}
	}

	void copyComponentIdentity(Scene* source, Scene* target) {
		auto components = source->registry.view<IdentityComponent>();
		for (auto& [srcEntity, srcIdentity] : components.each()) {
			entt::entity destEntity = target->entityMap.at(srcIdentity.uuid).handle;
			auto& destComponent = target->registry.emplace_or_replace<IdentityComponent>(destEntity, srcIdentity);
		}
	}

	template<typename T>
	void copyComponent(Scene* source, Scene* target) {
		auto components = source->registry.view<IdentityComponent, T>();
		for (auto& [srcEntity, srcIdentity, srcComponent] : components.each()) {
			entt::entity destEntity = target->entityMap.at(srcIdentity.uuid).handle;
			auto& destComponent = target->registry.emplace_or_replace<T>(destEntity, srcComponent);
		}
	}

	void copyScene(Scene* source, Scene* target) {
		// Copy main camera setting
		target->mainCameraEntityID = source->mainCameraEntityID;

		// Copy entity map
		auto identityComponents = source->registry.view<IdentityComponent>();
		for (auto& [entity, identityComponent] : identityComponents.each()) {
			Entity e = createEntityWithID(target, identityComponent.name, identityComponent.uuid);
			target->entityMap[identityComponent.uuid] = e;
		}

		// Copy components
		copyComponentIdentity(source, target);
		copyComponent<TransformComponent>(source, target);
		copyComponent<ModelComponent>(source, target);
		copyComponent<ScriptComponent>(source, target);
		copyComponent<PointLightComponent>(source, target);
		copyComponent<CameraComponent>(source, target);

		// Load script entities
		loadSceneScriptEntities(getActiveContext(), target);

		// Copy script data
		const auto& instanceData = getActiveContext()->instanceData;
		if (instanceData.find(target->uuid) != instanceData.end()) {
			copyEntityScriptData(getActiveContext(), source, target);
		}
	}

	Scene* createCopy(Scene* scene) {
		Scene* newScene = createScene();
		copyScene(scene, newScene);
		return newScene;
	}


	//----------------------------------------
	// SECTION: Components
	//----------------------------------------

	glm::vec3 getTransformPosition(const TransformComponent& transform) {
		return getTransformMatrixPosition(transform.matrix);
	}

	void setTransformPosition(TransformComponent& transform, glm::vec3 position) {
		transform.matrix = setTransformMatrixPosition(transform.matrix, position);
	}

	glm::vec3 getTransformMatrixPosition(const glm::mat4& matrix) {
		return matrix[3];
	}

	glm::mat4 setTransformMatrixPosition(const glm::mat4& matrix, glm::vec3 position) {
		glm::mat4 out = matrix;
		out[3][0] = position.x;
		out[3][1] = position.y;
		out[3][2] = position.z;
		return out;
	}

	glm::quat getTransformRotation(const TransformComponent& transform) {
		return getTransformMatrixRotation(transform.matrix);
	}

	void setTransformRotation(TransformComponent& transform, glm::quat rotation) {
		transform.matrix = setTransformMatrixRotation(transform.matrix, rotation);
	}

	glm::quat getTransformMatrixRotation(const glm::mat4& matrix) {
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, scale, rotation, translation, skew, perspective);
		return rotation;
	}

	glm::mat4 setTransformMatrixRotation(const glm::mat4& matrix, glm::quat rotation) {
		glm::vec3 scale;
		glm::quat oldRotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, scale, oldRotation, translation, skew, perspective);

		return matrix * glm::toMat4(glm::inverse(oldRotation) * rotation);
	}

	glm::vec3 getTransformScale(const TransformComponent& transform) {
		return getTransformMatrixScale(transform.matrix);
	}

	void setTransformScale(TransformComponent& transform, glm::vec3 scale) {
		transform.matrix = setTransformMatrixScale(transform.matrix, scale);
	}

	glm::vec3 getTransformMatrixScale(const glm::mat4& matrix) {
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, scale, rotation, translation, skew, perspective);

		return scale;
	}

	glm::mat4 setTransformMatrixScale(const glm::mat4& matrix, glm::vec3 scale) {
		glm::vec3 oldScale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, oldScale, rotation, translation, skew, perspective);

		return glm::scale(matrix, scale / oldScale);
	}

}
