#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "xenon/core/uuid.h"
#include "xenon/graphics/renderer.h"
#include "xenon/graphics/camera.h"
#include "xenon/graphics/environment.h"

namespace xe {
	
	//----------------------------------------
	// SECTION: Entity
	//----------------------------------------

	struct Scene;
	struct Entity {
		entt::entity handle;
		Scene* scene;

		template<typename T, typename... Args>
		T& addComponent(Args&&... args) {
			return scene->registry.emplace<T>(handle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& getComponent() const {
			return scene->registry.get<T>(handle);
		}

		template<typename T>
		bool hasComponent() const {
			return scene->registry.all_of<T>(handle);
		}

		template<typename T>
		void removeComponent() {
			scene->registry.remove<T>(handle);
		}

		operator bool() const { return handle != entt::null; }
		operator uint32_t() const { return (uint32_t)handle; }

		bool operator==(const Entity& other) const {
			return handle == other.handle && scene == other.scene;
		}

		bool operator!=(const Entity& other) const {
			return !(*this == other);
		}
	};

	//----------------------------------------
	// SECTION: Entity functions
	//----------------------------------------

	UUID getEntityID(Entity entity);


	//----------------------------------------
	// SECTION: Scene
	//----------------------------------------

	struct Scene {
		UUID uuid;
		entt::registry registry;
		std::unordered_map<UUID, Entity> entityMap;
		UUID mainCameraEntityID = UUID::None();
	};

	Scene* createScene();
	void destroyScene(Scene* scene);


	//----------------------------------------
	// SECTION: Scene functions
	//----------------------------------------

	Entity createEntity(Scene* scene, const std::string& name = "Entity");
	Entity createEntityWithID(Scene* scene, const std::string& name, const UUID& id);
	void removeEntity(Scene* scene, UUID id);

	Entity getEntityFromID(Scene* scene, UUID id);
	glm::mat4 getWorldMatrix(Entity entity);
	glm::mat4 getParentWorldMatrix(Entity entity);
	glm::mat4 toLocalMatrix(glm::mat4 matrix, Entity entity);
	
	void renderScene(Scene* scene, const Renderer& renderer, const Environment& environment);
	void renderSceneCustomCamera(Scene* scene, const Renderer& renderer, const Environment& environment, const Camera& camera, const glm::mat4& cameraTransform = glm::mat4(1.0f));
	void updateSceneModels(Scene* scene, bool isPlaying, float delta);

	void copyScene(Scene* source, Scene* target);
	Scene* createCopy(Scene* scene);

	//----------------------------------------
	// SECTION: Components
	//----------------------------------------

	struct SceneComponent {
		Scene* scene;
	};

	struct IdentityComponent {
		UUID uuid = UUID();
		std::string name = "";
		std::vector<uint64_t> tags = {};
	};

	struct TransformComponent {
		glm::mat4 matrix = glm::mat4(1.0f);
		UUID parent = UUID::None();
	};

	glm::vec3 getTransformPosition(const TransformComponent& transform);
	void setTransformPosition(TransformComponent& transform, glm::vec3 position);
	glm::vec3 getTransformMatrixPosition(const glm::mat4& matrix);
	glm::mat4 setTransformMatrixPosition(const glm::mat4& matrix, glm::vec3 position);

	glm::quat getTransformRotation(const TransformComponent& transform);
	void setTransformRotation(TransformComponent& transform, glm::quat rotation);
	glm::quat getTransformMatrixRotation(const glm::mat4& matrix);
	glm::mat4 setTransformMatrixRotation(const glm::mat4& matrix, glm::quat rotation);

	glm::vec3 getTransformScale(const TransformComponent& transform);
	void setTransformScale(TransformComponent& transform, glm::vec3 scale);
	glm::vec3 getTransformMatrixScale(const glm::mat4& matrix);
	glm::mat4 setTransformMatrixScale(const glm::mat4& matrix, glm::vec3 scale);

}
