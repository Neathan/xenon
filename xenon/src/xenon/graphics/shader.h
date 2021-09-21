#pragma once

#include <string>
#include <glm/glm.hpp>

#include "xenon/graphics/material.h"
#include "xenon/graphics/light.h"

namespace xe {

	//----------------------------------------
	// SECTION: Shader
	//----------------------------------------

	struct Shader {
		unsigned int programID;
	};

	Shader* loadShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	void destroyShader(Shader* shader);

	//----------------------------------------
	// SECTION: Shader functions
	//----------------------------------------

	void bindShader(const Shader& shader);
	void unbindShader();

	void loadInt(const Shader& shader, const char* name, int value);
	void loadFloat(const Shader& shader, const char* name, float value);
	void loadVec2(const Shader& shader, const char* name, glm::vec2 value);
	void loadVec3(const Shader& shader, const char* name, glm::vec3 value);
	void loadVec4(const Shader& shader, const char* name, glm::vec4 value);
	void loadMat4(const Shader& shader, const char* name, glm::mat4 value);

	void loadMaterial(const Shader& shader, const Material& material);
	void loadLight(const Shader& shader, const glm::vec3& position, const PointLight& light, int index);

}
