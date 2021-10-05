#version 460 core

in vec3 position;

out vec4 fragColor;

layout(binding = 0) uniform samplerCube iblSourceTexture;

const float PI = 3.14159265359;

void main() {
	vec3 N = normalize(position);
	vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));  // TODO: Remove, redundant

	float sampleDelta = 0.025;
	float nrSamples = 0.0;

	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
		for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
			irradiance += texture(iblSourceTexture, sampleVec).rgb * cos(theta) * sin(theta);
			++nrSamples;
		}
	}
	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	fragColor = vec4(irradiance, 1.0);

}