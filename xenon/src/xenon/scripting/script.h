#pragma once

#include <mono/jit/jit.h>

#include "xenon/scene/scene.h"

namespace xe {

	//---------------------------------------------------------------
	// [SECTION] Context pre-definition
	//---------------------------------------------------------------

	struct ScriptContext;


	//---------------------------------------------------------------
	// [SECTION] Script class
	//---------------------------------------------------------------

	struct ScriptClass {
		std::string moduleName;
		std::string namespaceName;
		std::string className;

		MonoClass* monoClass = nullptr;
		MonoMethod* constructor = nullptr;

		MonoMethod* onStart = nullptr;
		MonoMethod* onUpdate = nullptr;
	};

	void loadScriptClassMethods(ScriptContext* context, ScriptClass& scriptClass);


	//---------------------------------------------------------------
	// [SECTION] Class field management
	//---------------------------------------------------------------

	enum class FieldType {
		None = 0,
		Float,
		Int,
		UnsignedInt,
		String,
		Vec2,
		Vec3,
		Vec4
		// TODO: ClassReference
		// TODO: Bool
	};

	struct Instance; // Implemented in "Instance management"

	struct Field {
		std::string name;
		std::string typeName;
		FieldType type;

		Instance* instance = nullptr;
		MonoClassField* monoClassField = nullptr;
		uint8_t* storedValueBuffer = nullptr;

		// TODO: Research if this has implications (DOD)
		Field(const std::string& name, const std::string& typeName, FieldType type);
		Field(const Field&) = delete;
		Field(Field&& other) noexcept;
		~Field();
	};

	using FieldMap = std::unordered_map<std::string, Field>;
	using ModuleFieldMap = std::unordered_map<std::string, std::unordered_map<std::string, Field>>;

	bool isRuntimeAvailable(const Field& field);
	FieldMap& getInstanceFields(ScriptContext* context, Entity entity);

	void setStoredValueRaw(Field& field, void* value);
	void* getStoredValueRaw(const Field& field);
	void setRuntimeValueRaw(Field& field, void* value);
	void* getRuntimeValueRaw(const Field& field);

	uint8_t* allocateBuffer(FieldType type);
	void getStoredValueI(const Field& field, void* target);
	void setStoredValueI(Field& field, void* value);
	void getRuntimeValueI(const Field& field, void* target);
	void setRuntimeValueI(Field& field, void* value);

	template<typename T>
	T getStoredValue(const Field& field) {
		T value;
		getStoredValueI(field, &value);
		return value;
	}

	template<typename T>
	void setStoredValue(Field& field, T value) {
		setStoredValueI(field, &value);
	}

	template<typename T>
	T getRuntimeValue(const Field& field) {
		T value;
		getRuntimeValueI(field, &value);
		return value;
	}

	template<typename T>
	void setRuntimeValue(Field& field, T value) {
		setRuntimeValueI(field, &value);
	}

	//---------------------------------------------------------------
	// [SECTION] Instance management
	//---------------------------------------------------------------

	struct Instance {
		ScriptClass* scriptClass = nullptr;

		uint32_t handle = 0;
		Scene* scene = nullptr;
	};

	struct InstanceData {
		Instance instance;
		ModuleFieldMap moduleFieldMap;
	};

	MonoObject* getInstanceMonoObject(const Instance& instance);


	//---------------------------------------------------------------
	// [SECTION] Component
	//---------------------------------------------------------------

	struct ScriptComponent {
		std::string moduleName;
	};


	//---------------------------------------------------------------
	// [SECTION] Scene & entity
	//---------------------------------------------------------------

	bool loadScriptEntity(ScriptContext* context, Entity entity);
	void unloadScriptEntity(ScriptContext* context, Entity entity);
	void unloadScriptEntity(ScriptContext* context, Entity entity, const std::string& moduleName);

	void initScriptEntity(ScriptContext* context, Entity entity);

	void cleanScriptEntity(ScriptContext* context, Entity entity);

	void destroyScriptScene(ScriptContext* context, Scene* scene);

	void loadSceneScriptEntities(ScriptContext* context, Scene* scene);
	void unloadSceneScriptEntities(ScriptContext* context, Scene* scene);

	void initSceneScriptEntities(ScriptContext* context, Scene* scene);

	void cleanSceneScriptEntities(ScriptContext* context, Scene* scene);

	void updateScriptEntities(ScriptContext* context, float delta);


	//---------------------------------------------------------------
	// [SECTION] Context
	//---------------------------------------------------------------

	struct ScriptContext {
		MonoDomain* globalDomain = nullptr;
		MonoDomain* scriptDomain = nullptr;

		MonoAssembly* coreAssembly = nullptr;
		MonoImage* coreAssemblyImage = nullptr;

		MonoAssembly* scriptAssembly = nullptr;
		MonoImage* scriptImage = nullptr;

		Scene* scene = nullptr;

		// ModuleName, ScriptClass
		std::map<std::string, ScriptClass> scriptClasses;

		// Scene UUID, <Entity UUID, Instance Data>
		std::unordered_map<UUID, std::unordered_map<UUID, InstanceData>> instanceData;

		std::unordered_map<MonoType*, std::function<bool(Entity&)>> hasComponentFuncs;
		std::unordered_map<MonoType*, std::function<void(Entity&)>> createComponentFuncs;
	};

	ScriptContext* createScriptContext(const std::string& name, const std::string& assemblyRootPath = "mono/lib");
	void destroyScriptContext(ScriptContext* context);
	void setActiveContext(ScriptContext* context);
	ScriptContext* getActiveContext();

	void loadCoreAssembly(ScriptContext* context, const std::string& path = "Xenon.dll");
	void loadScriptAssembly(ScriptContext* context, const std::string& path);

	bool moduleExists(ScriptContext* context, const std::string& moduleName);

	void copyEntityScriptData(ScriptContext* context, Scene* source, Scene* target);
}
