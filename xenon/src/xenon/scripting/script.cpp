#include "script.h"

#include <fstream>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>

#include <glm/gtx/quaternion.hpp>

#include "xenon/core/log.h"
#include "xenon/core/assert.h"


namespace xe {

	//---------------------------------------------------------------
	// [SECTION] Global context
	//---------------------------------------------------------------

	static ScriptContext* s_activeContext = nullptr;

	void attachInternals(ScriptContext* context);


	//---------------------------------------------------------------
	// [SECTION] Internal functions
	//---------------------------------------------------------------

	MonoAssembly* loadAssembly(const std::string& path) {
		// Open file
		std::ifstream fileStream(path, std::ios::binary | std::ios::ate); // Start at end
		std::streamsize size = fileStream.tellg();
		fileStream.seekg(0, std::ios::beg); // Seek to start

		// Allocate memory
		char* fileData = new char[size];
		if (!fileData) {
			XE_LOG_ERROR_F("SCRIPT: Failed to allocate {} bytes of memory for assembly: {}", size, path);
			return nullptr;
		}

		// Read data
		if (!fileStream.read(fileData, size)) {
			XE_LOG_ERROR_F("SCRIPT: Failed to read assembly content from: {}", path);
			delete[] fileData;
			return nullptr;
		}

		// Load assembly from binary data
		MonoImageOpenStatus imageStatus;
		MonoImage* image = mono_image_open_from_data_full(fileData, size, true, &imageStatus, false);
		if (imageStatus != MONO_IMAGE_OK) {
			XE_LOG_ERROR_F("SCRIPT: Failed to load assembly image from: {}", path);
			delete[] fileData;
			return nullptr;
		}
		else {
			XE_LOG_TRACE_F("SCRIPT: Loaded assembly image from file: {}", path);
		}


		MonoAssembly* assembly = mono_assembly_load_from_full(image, path.c_str(), &imageStatus, false);

		delete[] fileData;
		mono_image_close(image);

		if (!assembly) {
			XE_LOG_ERROR_F("SCRIPT: Failed to load assembly from image: {}", path);
			return nullptr;
		}
		else {
			XE_LOG_TRACE_F("SCRIPT: Loaded assembly from image: {}", path);
		}

		return assembly;
	}

	MonoImage* getAssemblyImage(MonoAssembly* assembly) {
		MonoImage* image = mono_assembly_get_image(assembly);
		if (!image) {
			XE_LOG_ERROR("SCRIPT: Failed to get image from assembly");
		}
		return image;
	}

	MonoMethod* getMethod(MonoImage* image, const std::string& methodDesc) {
		MonoMethodDesc* desc = mono_method_desc_new(methodDesc.c_str(), false);
		if (!desc) {
			XE_LOG_ERROR_F("SCRIPT: Failed to create method description from give string: {}", methodDesc);
			return nullptr;
		}

		MonoMethod* method = mono_method_desc_search_in_image(desc, image);
		if (!method) {
			XE_LOG_ERROR_F("SCRIPT: Failed to find method for give description: {}", methodDesc);
			return nullptr;
		}

		return method;
	}

	MonoClass* getClass(MonoImage* image, const ScriptClass& scriptClass) {
		MonoClass* monoClass = mono_class_from_name(image, scriptClass.namespaceName.c_str(), scriptClass.className.c_str());
		if (!monoClass) {
			XE_LOG_ERROR_F("SCRIPT: Failed to get class from: {}", scriptClass.moduleName);
		}
		return monoClass;
	}

	uint32_t createInstance(ScriptContext* context, const ScriptClass& scriptClass) {
		MonoObject* instance = mono_object_new(context->scriptDomain, scriptClass.monoClass);
		if (!instance) {
			XE_LOG_ERROR_F("SCRIPT: Failed to create object instance of: {}", scriptClass.moduleName);
		}
		mono_runtime_object_init(instance);
		return mono_gchandle_new(instance, false);
	}

	MonoObject* invokeMethod(MonoObject* object, MonoMethod* method, void** params) {
		MonoObject* pException = nullptr;
		MonoObject* result = mono_runtime_invoke(method, object, params, &pException);
		// TODO: manage exception
		return result;
	}


	//---------------------------------------------------------------
	// [SECTION] Script class
	//---------------------------------------------------------------

	void loadScriptClassMethods(ScriptContext* context, ScriptClass& scriptClass) {
		scriptClass.constructor = getMethod(context->coreAssemblyImage, "Xenon.Entity:.ctor(ulong)");
		scriptClass.onStart = getMethod(context->scriptImage, (scriptClass.moduleName + ":Start ()").c_str());
		scriptClass.onUpdate = getMethod(context->scriptImage, (scriptClass.moduleName + ":Update (single)").c_str());
	}


	//---------------------------------------------------------------
	// [SECTION] Class field management
	//---------------------------------------------------------------

	static uint32_t getFieldSize(FieldType type) {
		switch (type) {
			case FieldType::Float:			return 4;
			case FieldType::Int:			return 4;
			case FieldType::UnsignedInt:	return 4;
				// case FieldType::String:		return 8; // TODO: Implement
			case FieldType::Vec2:			return 4 * 2;
			case FieldType::Vec3:			return 4 * 3;
			case FieldType::Vec4:			return 4 * 4;
			//case FieldType::ClassReference: return 4;
		}
		XE_ASSERT(false); // Invalid FieldType
		return 0;
	}

	static FieldType getFieldType(MonoType* monoType) {
		int type = mono_type_get_type(monoType);
		switch (type) {
			case MONO_TYPE_R4: return FieldType::Float;
			case MONO_TYPE_I4: return FieldType::Int;
			case MONO_TYPE_U4: return FieldType::UnsignedInt;
			case MONO_TYPE_STRING: return FieldType::String;
			// case MONO_TYPE_CLASS: return FieldType::ClassReference;
			case MONO_TYPE_VALUETYPE: {
				char* name = mono_type_get_name(monoType);
				if (strcmp(name, "Xenon.Vector2") == 0) return FieldType::Vec2;
				if (strcmp(name, "Xenon.Vector3") == 0) return FieldType::Vec3;
				if (strcmp(name, "Xenon.Vector4") == 0) return FieldType::Vec4;
			}
		}
		return FieldType::None;
	}

	void copyStoredValueToRuntime(const Field& field) {
		MonoObject* object = getInstanceMonoObject(*field.instance);
		XE_ASSERT(object);

// 		if (type == FieldType::ClassReference) {
// 			// Create managed object
// 			void* params[] = {
// 				&storedValueBuffer
// 			};
// 			// TODO: implement
// 		}
// 		else {
		mono_field_set_value(object, field.monoClassField, field.storedValueBuffer);
// 		}
	}

	Field::Field(const std::string& name, const std::string& typeName, FieldType type)
		: name(name), typeName(typeName), type(type) {
		storedValueBuffer = allocateBuffer(type);
	}

	Field::Field(Field&& other) noexcept {
		name = std::move(other.name);
		typeName = std::move(other.typeName);
		type = other.type;
		instance = other.instance;
		monoClassField = other.monoClassField;
		storedValueBuffer = other.storedValueBuffer;

		other.instance = nullptr;
		other.monoClassField = nullptr;
		other.storedValueBuffer = nullptr;
	}

	Field::~Field() {
		delete[] storedValueBuffer;
	}

	bool isRuntimeAvailable(const Field& field) {
		return field.instance->handle != 0;
	}

	FieldMap& getInstanceFields(ScriptContext* context, Entity entity) {
		XE_ASSERT(entity.hasComponent<ScriptComponent>());

		ScriptComponent& scriptComponent = entity.getComponent<ScriptComponent>();

		return context->instanceData.at(context->scene->uuid).at(getEntityID(entity)).moduleFieldMap.at(scriptComponent.moduleName);
	}

	void setStoredValueRaw(Field& field, void* value) {
// 		if (field.type == FieldType::ClassReference) {
// 			storedValueBuffer = (uint8_t*)value;
// 		}
// 		else {
		memcpy(field.storedValueBuffer, value, getFieldSize(field.type));
// 		}
	}

	void* getStoredValueRaw(const Field& field) {
		return field.storedValueBuffer;
	}

	void setRuntimeValueRaw(Field& field, void* value) {
		MonoObject* object = getInstanceMonoObject(*field.instance);
		XE_ASSERT(object);
		mono_field_set_value(object, field.monoClassField, value);
	}

	void* getRuntimeValueRaw(const Field& field) {
		MonoObject* object = getInstanceMonoObject(*field.instance);
		XE_ASSERT(object);
// 		if (type == FieldType::ClassReference) {
// 			MonoObject* obj;
// 			mono_field_get_value(instance->getMonoObject(), monoClassField, &obj);
// 
// 			if (!obj) {
// 				return nullptr;
// 			}
// 
// 			MonoClassField* field = mono_class_get_field_from_name(mono_object_get_class(obj), "m_UnmanagedInstance");
// 			int* value;
// 			mono_field_get_value(obj, field, &value);
// 			return value;
// 		}
// 		else {
		uint8_t* value;
		mono_field_get_value(object, field.monoClassField, &value);
		return value;
// 		}
	}

	uint8_t* allocateBuffer(FieldType type) {
		uint32_t size = getFieldSize(type);
		uint8_t* buffer = new uint8_t[size];
		memset(buffer, 0, size);
		return buffer;
	}

	void getStoredValueI(const Field& field, void* target) {
		memcpy(target, field.storedValueBuffer, getFieldSize(field.type));
	}

	void setStoredValueI(Field& field, void* value) {
// 		if (field.type == FieldType::ClassReference) {
// 			// storedValueBuffer = (uint8_t*)value;
// 			XE_LOG_DEBUG("setStoredValueI ClassReference not implemented");
// 		}
// 		else {
			memcpy(field.storedValueBuffer, value, getFieldSize(field.type));
// 		}
	}

	void getRuntimeValueI(const Field& field, void* target) {
		MonoObject* object = getInstanceMonoObject(*field.instance);
		XE_ASSERT(object);
		mono_field_get_value(object, field.monoClassField, target);
	}

	void setRuntimeValueI(Field& field, void* value) {
		MonoObject* object = getInstanceMonoObject(*field.instance);
		XE_ASSERT(object);
		mono_field_set_value(object, field.monoClassField, value);
	}


	//---------------------------------------------------------------
	// [SECTION] Instance management
	//---------------------------------------------------------------

	MonoObject* getInstanceMonoObject(const Instance& instance) {
		XE_ASSERT(instance.handle); // Uninitialized
		return mono_gchandle_get_target(instance.handle);
	}


	//---------------------------------------------------------------
	// [SECTION] Scene & entity
	//---------------------------------------------------------------

	bool loadScriptEntity(ScriptContext* context, Entity entity) {
		XE_ASSERT(entity.hasComponent<ScriptComponent>());

		ScriptComponent& scriptComponent = entity.getComponent<ScriptComponent>();
		std::string& moduleName = scriptComponent.moduleName;

		if (moduleName.empty()) {
			XE_LOG_ERROR_F("SCRIPT: Script component has empty module name: {}", xe::getEntityID(entity));
			return false;
		}

		if (!moduleExists(context, moduleName)) {
			XE_LOG_TRACE_F("SCRIPT: Could not find module named: {}", moduleName);
			return false;
		}

		// Create class
		ScriptClass& scriptClass = context->scriptClasses[moduleName];
		scriptClass.moduleName = moduleName;
		
		// Extract class and namespace name (if any)
		if (moduleName.find('.') != std::string::npos) {
			scriptClass.namespaceName = moduleName.substr(0, moduleName.find_last_of('.'));
			scriptClass.className = moduleName.substr(moduleName.find_last_of('.') + 1);
		}
		else {
			scriptClass.className = moduleName;
		}

		// Load class and retrieve functions
		scriptClass.monoClass = getClass(context->scriptImage, scriptClass);
		loadScriptClassMethods(context, scriptClass);

		// Create instance
		InstanceData& instanceData = context->instanceData[entity.scene->uuid][getEntityID(entity)];
		Instance& instance = instanceData.instance;
		instance.scriptClass = &scriptClass;
	
		// Manage fields
		FieldMap& fieldMap = instanceData.moduleFieldMap[moduleName];

		FieldMap oldFields;
		oldFields.reserve(fieldMap.size());
		for (auto& [fieldName, field] : fieldMap) {
			oldFields.emplace(fieldName, std::move(field));
		}
		fieldMap.clear();

		MonoClassField* iter;
		void* ptr = 0;
		while ((iter = mono_class_get_fields(scriptClass.monoClass, &ptr)) != nullptr) {
			const char* name = mono_field_get_name(iter);
			uint32_t flags = mono_field_get_flags(iter);
			if ((flags & MONO_FIELD_ATTR_PUBLIC) == 0) { // Ignore non-public fields (for now)
				continue;
			}

			MonoType* monoFieldType = mono_field_get_type(iter);
			FieldType fieldType = getFieldType(monoFieldType);

// 			if (fieldType == FieldType::ClassReference) {
// 				continue;
// 			}

			// TODO: Attributes
			MonoCustomAttrInfo* attr = mono_custom_attrs_from_field(scriptClass.monoClass, iter);

			char* typeName = mono_type_get_name(monoFieldType);

			if (oldFields.find(name) != oldFields.end()) {
				fieldMap.emplace(name, std::move(oldFields.at(name)));
				fieldMap.at(name).monoClassField = iter;  // Could differ if assembly has been reloaded
			}
			else {
				Field field(name, typeName, fieldType);
				field.instance = &instance;
				field.monoClassField = iter;
				fieldMap.emplace(name, std::move(field));
				// TODO: ClassReference
			}
		}

		return true;
	}

	void unloadScriptEntity(ScriptContext* context, Entity entity) {
		XE_ASSERT(entity.hasComponent<ScriptComponent>());

		ScriptComponent& scriptComponent = entity.getComponent<ScriptComponent>();
		unloadScriptEntity(context, entity, scriptComponent.moduleName);
	}

	void unloadScriptEntity(ScriptContext* context, Entity entity, const std::string& moduleName) {
		InstanceData& instanceData = context->instanceData.at(entity.scene->uuid).at(getEntityID(entity));

		if (instanceData.moduleFieldMap.find(moduleName) != instanceData.moduleFieldMap.end()) {
			instanceData.moduleFieldMap.erase(moduleName);
		}
	}

	void initScriptEntity(ScriptContext* context, Entity entity) {
		XE_ASSERT(entity.hasComponent<ScriptComponent>());

		if (context->instanceData.find(entity.scene->uuid) == context->instanceData.end()) {
			return;
		}

		ScriptComponent& scriptComponent = entity.getComponent<ScriptComponent>();
		std::string& moduleName = scriptComponent.moduleName;

		InstanceData& instanceData = context->instanceData.at(entity.scene->uuid).at(getEntityID(entity));
		Instance& instance = instanceData.instance;
		XE_ASSERT(instance.scriptClass); // Check if its been loaded

		// TODO: Check what happens to any existing instance (free, how?)
		instance.handle = createInstance(context, *instance.scriptClass);

		UUID id = getEntityID(entity); // NOTE: Scoped value
		void* params[]{ &id };
		invokeMethod(getInstanceMonoObject(instance), instance.scriptClass->constructor, params);

		// Set (public) fields
		ModuleFieldMap& moduleFieldMap = instanceData.moduleFieldMap;
		if (moduleFieldMap.find(moduleName) != moduleFieldMap.end()) {
			FieldMap& fields = moduleFieldMap.at(moduleName);
			for (auto& [name, field] : fields) {
				copyStoredValueToRuntime(field);
			}
		}

		// TODO: OnAwake
	}

	void cleanScriptEntity(ScriptContext* context, Entity entity) {
		context->instanceData.at(entity.scene->uuid).erase(getEntityID(entity));
		// TODO: Check if MonoObject needs to be destroyed
	}

	void destroyScriptScene(ScriptContext* context, Scene* scene) {
		context->instanceData.erase(scene->uuid);
	}

	void loadSceneScriptEntities(ScriptContext* context, Scene* scene) {
		auto scripts = scene->registry.view<ScriptComponent>();
		for (auto& [uuid, script] : scripts.each()) {
			loadScriptEntity(context, { uuid, scene });
		}
	}

	void unloadSceneScriptEntities(ScriptContext* context, Scene* scene) {
		auto scripts = scene->registry.view<ScriptComponent>();
		for (auto& [uuid, script] : scripts.each()) {
			unloadScriptEntity(context, { uuid, scene });
		}
	}

	void initSceneScriptEntities(ScriptContext* context, Scene* scene) {
		auto scripts = scene->registry.view<ScriptComponent>();
		for (auto& [uuid, script] : scripts.each()) {
			initScriptEntity(context, { uuid, scene });
		}
	}

	void cleanSceneScriptEntities(ScriptContext* context, Scene* scene) {
		auto scripts = scene->registry.view<ScriptComponent>();
		for (auto& [uuid, script] : scripts.each()) {
			cleanScriptEntity(context, { uuid, scene });
		}
	}

	void startScriptEntities(ScriptContext* context) {
		auto scripts = context->scene->registry.view<ScriptComponent>();
		for (auto& [uuid, script] : scripts.each()) {
			if (context->instanceData.find(context->scene->uuid) == context->instanceData.end()) {
				continue;
			}

			Instance& instance = context->instanceData.at(context->scene->uuid).at(getEntityID({ uuid, context->scene })).instance;
			if (instance.handle && instance.scriptClass->onUpdate) {
				invokeMethod(getInstanceMonoObject(instance), instance.scriptClass->onStart, nullptr);
			}
		}
	}

	void updateScriptEntities(ScriptContext* context, float delta) {
		auto scripts = context->scene->registry.view<ScriptComponent>();
		for (auto& [uuid, script] : scripts.each()) {
			if (context->instanceData.find(context->scene->uuid) == context->instanceData.end()) {
				continue;
			}

			Instance& instance = context->instanceData.at(context->scene->uuid).at(getEntityID({ uuid, context->scene })).instance;
			if (instance.handle && instance.scriptClass->onUpdate) {
				void* params[] = { &delta };
				invokeMethod(getInstanceMonoObject(instance), instance.scriptClass->onUpdate, params);
			}
		}
	}


	//---------------------------------------------------------------
	// [SECTION] Context
	//---------------------------------------------------------------

	ScriptContext* createScriptContext(const std::string& name, const std::string& assemblyRootPath) {
		ScriptContext* context = new ScriptContext();

		if (!assemblyRootPath.empty()) {
			// TODO: Research if this could be omitted
			mono_set_assemblies_path(assemblyRootPath.c_str());
		}

		context->globalDomain = mono_jit_init(name.c_str());
		context->scriptDomain = mono_domain_create_appdomain("XEScript", nullptr);

		s_activeContext = context;
		return context;
	}

	void destroyScriptContext(ScriptContext* context) {
		// TODO: Find best way to properly shutdown mono domain
		//  see https://www.mono-project.com/docs/advanced/embedding/
		//  for why current implementation can cause problems (mono_jit_init)
		mono_jit_cleanup(context->scriptDomain);

		if (context == s_activeContext) {
			s_activeContext = nullptr;
		}

		delete context;
	}

	void setActiveContext(ScriptContext* context) {
		s_activeContext = context;
	}

	ScriptContext* getActiveContext() {
		return s_activeContext;
	}

	void loadCoreAssembly(ScriptContext* context, const std::string& path) {
		// TODO: Clean up
		context->coreAssembly = loadAssembly(path);
		context->coreAssemblyImage = getAssemblyImage(context->coreAssembly);
	}

	void loadScriptAssembly(ScriptContext* context, const std::string& path) {
		MonoDomain* domain = nullptr;
		bool clean = false;
		if (context->scriptDomain) {
			domain = mono_domain_create_appdomain("XEScript", nullptr);
			mono_domain_set(domain, false);
			clean = true;
		}

		// TODO: Figure out why this causes types issues if done before domain is set
		loadCoreAssembly(context);

		MonoAssembly* assembly = loadAssembly(path);
		MonoImage* image = getAssemblyImage(assembly);

		attachInternals(context);

		if (clean) {
			mono_domain_unload(context->scriptDomain);
			context->scriptDomain = domain;
		}

		context->scriptAssembly = assembly;
		context->scriptImage = image;
	}

	bool moduleExists(ScriptContext* context, const std::string& moduleName) {
		std::string namespaceName, className;
		if (moduleName.find('.') != std::string::npos) {
			namespaceName = moduleName.substr(0, moduleName.find_first_of('.'));
			className = moduleName.substr(moduleName.find_first_of('.') + 1);
		}
		else {
			className = moduleName;
		}

		return mono_class_from_name(context->scriptImage, namespaceName.c_str(), className.c_str());
	}

	void copyEntityScriptData(ScriptContext* context, Scene* source, Scene* target) {
		XE_ASSERT(context->instanceData.find(source->uuid) != context->instanceData.end());
		XE_ASSERT(context->instanceData.find(target->uuid) != context->instanceData.end());

		auto& targetEntityMap = context->instanceData.at(target->uuid);
		auto& sourceEntityMap = context->instanceData.at(source->uuid);

		for (auto& [entityID, instanceData] : sourceEntityMap) {
			for (auto& [moduleName, sourceFieldMap] : sourceEntityMap[entityID].moduleFieldMap) {

				auto& targetModuleFieldMap = targetEntityMap[entityID].moduleFieldMap;
				for (auto& [fieldName, field] : sourceFieldMap) {

					XE_ASSERT(targetModuleFieldMap.find(moduleName) != targetModuleFieldMap.end());
					auto& fieldMap = targetModuleFieldMap.at(moduleName);

					XE_ASSERT(fieldMap.find(fieldName) != fieldMap.end());
					setStoredValueRaw(fieldMap.at(fieldName), field.storedValueBuffer);
				}
			}
		}
	}


	//---------------------------------------------------------------
	// [SECTION] Core internals
	//---------------------------------------------------------------

	void entityCreateComponent(uint64_t entityID, void* type) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);
		s_activeContext->createComponentFuncs.at(monoType)(entity);
	}

	bool entityHasComponent(uint64_t entityID, void* type) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);
		return s_activeContext->hasComponentFuncs.at(monoType)(entity);
	}

	uint64_t entityFindEntityByTag(MonoString* tag) {
		// TODO: implement
		XE_LOG_ERROR("entityFindEntityByTag not implemented");
		return 0;
	}


	//---------------------------------------------------------------
	// [SUB-SECTION] Transform internals
	//---------------------------------------------------------------

	void transformComponentGetTransform(uint64_t entityID, TransformComponent* outTransform) {
		// TODO: This should not currently work, fix that.
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		*outTransform = entity.getComponent<TransformComponent>();
	}

	void transformComponentSetTransform(uint64_t entityID, TransformComponent* inTransform) {
		// TODO: This should not currently work, fix that.
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		entity.getComponent<TransformComponent>() = *inTransform;
	}

	void transformComponentGetPosition(uint64_t entityID, glm::vec3* outPosition) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		*outPosition = getTransformPosition(entity.getComponent<TransformComponent>());
	}

	void transformComponentSetPosition(uint64_t entityID, glm::vec3* inPosition) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		setTransformPosition(entity.getComponent<TransformComponent>(), *inPosition);
	}

	void transformComponentGetRotation(uint64_t entityID, glm::vec3* outRotation) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		*outRotation = glm::eulerAngles(getTransformRotation(entity.getComponent<TransformComponent>())); // TODO: Change script core to use quaternions
	}

	void transformComponentSetRotation(uint64_t entityID, glm::vec3* inRotation) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		setTransformRotation(entity.getComponent<TransformComponent>(), *inRotation);
	}

	void transformComponentGetScale(uint64_t entityID, glm::vec3* outScale) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		*outScale = getTransformScale(entity.getComponent<TransformComponent>());
	}

	void transformComponentSetScale(uint64_t entityID, glm::vec3* inScale) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		setTransformScale(entity.getComponent<TransformComponent>(), *inScale);
	}

	//---------------------------------------------------------------
	// [SUB-SECTION] Point light internals
	//---------------------------------------------------------------

	void pointLightComponentGetColor(uint64_t entityID, glm::vec3* outColor) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		*outColor = entity.getComponent<PointLightComponent>().color;
	}

	void pointLightComponentSetColor(uint64_t entityID, glm::vec3* inColor) {
		Entity entity = getEntityFromID(s_activeContext->scene, entityID);
		entity.getComponent<PointLightComponent>().color = *inColor;
	}

	//---------------------------------------------------------------
	// [SUB-SECTION] Attachment
	//---------------------------------------------------------------


	#define COMPONENT_REGISTER_TYPE_TEMPLATE(Type, context, Namespace) { \
		MonoType* type = mono_reflection_type_from_name("Xenon." #Type, context->coreAssemblyImage); \
		if (type) { \
			uint32_t id = mono_type_get_type(type); \
			context->hasComponentFuncs[type] = [](Entity& entity) { return entity.hasComponent<Namespace Type>(); }; \
			context->createComponentFuncs[type] = [](Entity& entity) { entity.addComponent<Namespace Type>(); }; \
		} \
		else { \
			XE_LOG_ERROR("SCRIPT: No C# component class found for: " #Type); \
		}\
	}

	void attachInternals(ScriptContext* context) {
		// Types
		COMPONENT_REGISTER_TYPE_TEMPLATE(IdentityComponent, context, );
		COMPONENT_REGISTER_TYPE_TEMPLATE(TransformComponent, context, );
		COMPONENT_REGISTER_TYPE_TEMPLATE(ScriptComponent, context, );
		COMPONENT_REGISTER_TYPE_TEMPLATE(PointLightComponent, context, );

		// Native functions
		mono_add_internal_call("Xenon.Entity::CreateComponent_Native", entityCreateComponent);
		mono_add_internal_call("Xenon.Entity::HasComponent_Native", entityHasComponent);
		mono_add_internal_call("Xenon.Entity::FindEntityByTag_Native", entityFindEntityByTag);

		mono_add_internal_call("Xenon.TransformComponent::GetTransform_Native", transformComponentGetTransform);
		mono_add_internal_call("Xenon.TransformComponent::SetTransform_Native", transformComponentSetTransform);
		mono_add_internal_call("Xenon.TransformComponent::GetPosition_Native", transformComponentGetPosition);
		mono_add_internal_call("Xenon.TransformComponent::SetPosition_Native", transformComponentSetPosition);
		mono_add_internal_call("Xenon.TransformComponent::GetRotation_Native", transformComponentGetRotation);
		mono_add_internal_call("Xenon.TransformComponent::SetRotation_Native", transformComponentSetRotation);
		mono_add_internal_call("Xenon.TransformComponent::GetScale_Native", transformComponentGetScale);
		mono_add_internal_call("Xenon.TransformComponent::SetScale_Native", transformComponentSetScale);

		mono_add_internal_call("Xenon.PointLightComponent::GetColor_Native", pointLightComponentGetColor);
		mono_add_internal_call("Xenon.PointLightComponent::SetColor_Native", pointLightComponentSetColor);
	}



}
