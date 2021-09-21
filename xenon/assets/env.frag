#version 460 core

in vec3 position;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out int fragObjectID;

layout(binding = 0) uniform samplerCube iblSourceTexture;

void main() {
	vec3 envColor = texture(iblSourceTexture, position).rgb;

	// HDR tonemap and gamma correct
	envColor = envColor / (envColor + vec3(1.0));
	envColor = pow(envColor, vec3(1.0/2.2)); 

	fragColor = vec4(envColor, 1.0);
	fragObjectID = 0;
}