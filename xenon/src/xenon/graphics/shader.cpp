#include "shader.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include "xenon/core/log.h"
#include "xenon/core/filesystem.h"

namespace xe {

	//----------------------------------------
	// SECTION: Shader
	//----------------------------------------

	GLuint compileShader(const char* shaderSource, GLenum shaderType) {
		GLuint shader = glCreateShader(shaderType);

		GLint compileResult = GL_FALSE;
		GLint infoLoglength;

		XE_LOG_TRACE("SHADER: Compiling shader");

		const char* shaderSourcePointer = shaderSource;
		glShaderSource(shader, 1, &shaderSourcePointer, NULL);
		glCompileShader(shader);

		glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLoglength);

		if (infoLoglength > 0) {
			std::vector<char> shaderErrorMessage((size_t)infoLoglength + 1);
			glGetShaderInfoLog(shader, infoLoglength, NULL, &shaderErrorMessage[0]);
			if (compileResult != GL_TRUE) {
				XE_LOG_ERROR_F("SHADER: Compilation error: {}", &shaderErrorMessage[0]);
				return -1;
			}
		}
		return shader;
	}

	bool checkStatus(GLuint programID, GLenum type) {
		GLint statusResult;
		GLint infoLogLength;
		glGetProgramiv(programID, type, &statusResult);
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);

		if (infoLogLength > 0) {
			std::vector<char> programErrorMessage((size_t)infoLogLength + 1);
			glGetProgramInfoLog(programID, infoLogLength, nullptr, &programErrorMessage[0]);

			if (statusResult != GL_TRUE) {
				XE_LOG_ERROR_F("SHADER: Status ({}) error:\n{}", type, &programErrorMessage[0]);
				glDeleteProgram(programID);
				return false;
			}
		}
		return true;
	}

	Shader* loadShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
		std::string vSource;
		std::string fSource;
		if (!loadTextResource(vertexShaderPath, vSource) || !loadTextResource(fragmentShaderPath, fSource)) {
			XE_LOG_ERROR_F("SHADER: Failed to load shader");
			return nullptr;
		}

		GLuint vertexShader = compileShader(vSource.c_str(), GL_VERTEX_SHADER);
		GLuint fragmentShader = compileShader(fSource.c_str(), GL_FRAGMENT_SHADER);

		if (vertexShader == -1 || fragmentShader == -1) {
			XE_LOG_ERROR_F("SHADER: Failed to compile shader");
			return nullptr;
		}

		GLuint programID = glCreateProgram();
		glAttachShader(programID, vertexShader);
		glAttachShader(programID, fragmentShader);
		glLinkProgram(programID);
		if (!checkStatus(programID, GL_LINK_STATUS)) {
			glDeleteProgram(programID);
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			return nullptr;
		}

		glValidateProgram(programID);
		if (!checkStatus(programID, GL_VALIDATE_STATUS)) {
			glDeleteProgram(programID);
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			return nullptr;
		}

		glDetachShader(programID, vertexShader);
		glDetachShader(programID, fragmentShader);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return new Shader{ programID };
	}

	void destroyShader(Shader* shader) {
		glDeleteProgram(shader->programID);
		delete shader;
	}


	//----------------------------------------
	// SECTION: Shader functions
	//----------------------------------------

	void bindShader(const Shader& shader) {
		glUseProgram(shader.programID);
	}

	void unbindShader() {
		glUseProgram(0);
	}


	void loadInt(const Shader& shader, const char* name, int value) {
		glUniform1i(glGetUniformLocation(shader.programID, name), value);
	}

	void loadFloat(const Shader& shader, const char* name, float value) {
		glUniform1f(glGetUniformLocation(shader.programID, name), value);
	}

	void loadVec2(const Shader& shader, const char* name, glm::vec2 value) {
		glUniform2f(glGetUniformLocation(shader.programID, name), value.x, value.y);
	}

	void loadVec3(const Shader& shader, const char* name, glm::vec3 value) {
		glUniform3f(glGetUniformLocation(shader.programID, name), value.x, value.y, value.z);
	}

	void loadVec4(const Shader& shader, const char* name, glm::vec4 value) {
		glUniform4f(glGetUniformLocation(shader.programID, name), value.x, value.y, value.z, value.w);
	}

	void loadMat4(const Shader& shader, const char* name, glm::mat4 value) {
		glUniformMatrix4fv(glGetUniformLocation(shader.programID, name), 1, GL_FALSE, glm::value_ptr(value));
	}


	void loadMaterial(const Shader& shader, const Material& material) {
		loadVec4(shader, "baseColorFactor", material.pbrMetallicRoughness.baseColorFactor);

		if (material.pbrMetallicRoughness.baseColorTexture) {
			glBindTextureUnit((int)material.pbrMetallicRoughness.baseColorTexture->type, material.pbrMetallicRoughness.baseColorTexture->textureID);
			loadInt(shader, "usingAlbedoMap", true);
		}
		else {
			loadInt(shader, "usingAlbedoMap", false);
		}

		loadFloat(shader, "metallicFactor", material.pbrMetallicRoughness.metallicFactor);
		loadFloat(shader, "roughnessFactor", material.pbrMetallicRoughness.roughnessFactor);

		if (material.pbrMetallicRoughness.metallicRoughnessTexture) {
			glBindTextureUnit((int)material.pbrMetallicRoughness.metallicRoughnessTexture->type, material.pbrMetallicRoughness.metallicRoughnessTexture->textureID);
			loadInt(shader, "usingMetallicRoughnessMap", true);
		}
		else {
			loadInt(shader, "usingMetallicRoughnessMap", false);
		}

		if (material.normalTexture) {
			glBindTextureUnit((int)material.normalTexture->type, material.normalTexture->textureID);
			loadInt(shader, "usingNormalMap", true);
		}
		else {
			loadInt(shader, "usingNormalMap", false);
		}

		if (material.occlusionTexture) {
			glBindTextureUnit((int)material.occlusionTexture->type, material.occlusionTexture->textureID);
			loadInt(shader, "usingAOMap", true);
		}
		else {
			loadInt(shader, "usingAOMap", false);
		}

		if (material.emissiveTexture) {
			glBindTextureUnit((int)material.emissiveTexture->type, material.emissiveTexture->textureID);
			loadInt(shader, "usingEmissiveMap", true);
		}
		else {
			loadInt(shader, "usingEmissiveMap", false);
		}

		loadVec3(shader, "emissiveFactor", material.emissiveFactor);
		loadInt(shader, "alphaMode", (int)material.alphaMode);
		loadFloat(shader, "alphaCutoff", material.alphaCutoff);
		loadInt(shader, "doubleSided", material.doubleSided);  // bool = int
	}

	void loadLight(const Shader& shader, const glm::vec3& position, const PointLightComponent& light, int index) {
		loadVec3(shader, ("pointLights[" + std::to_string(index) + "].position").c_str(), position);
		loadVec3(shader, ("pointLights[" + std::to_string(index) + "].color").c_str(), light.color);
	}
}

