#version 460 core

//---------------------------------------------------------------
// [SECTION] Input data & output variables
//---------------------------------------------------------------

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_tangent;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_textureCoord;
layout(location = 4) in vec4 in_joint;
layout(location = 5) in vec4 in_weight;

out vec4 position;
out vec3 tangent;
out vec3 normal;
out vec2 textureCoord;

out mat3 TBN;

uniform bool usingAttribJoints0;
uniform bool usingAttribWeights0;


//---------------------------------------------------------------
// [SECTION] Matricies
//---------------------------------------------------------------

uniform mat4 projection;
uniform mat4 view;
uniform mat4 transform;


//---------------------------------------------------------------
// [SECTION] Skin
//---------------------------------------------------------------

#define MAX_BONE_COUNT 128

uniform mat4 jointMatrices[MAX_BONE_COUNT];


//---------------------------------------------------------------
// [SECTION] Main program
//---------------------------------------------------------------

void main() {
	// Calculate skin matrix
	mat4 skinMatrix = mat4(1.0);
	if(usingAttribJoints0 && usingAttribWeights0) {
		skinMatrix =
			in_weight.x * jointMatrices[int(in_joint.x)] +
			in_weight.y * jointMatrices[int(in_joint.y)] +
			in_weight.z * jointMatrices[int(in_joint.z)] +
			in_weight.w * jointMatrices[int(in_joint.w)];
	}

	// Apply transformation on normal
	mat3 vectorTransform =  mat3(transpose(inverse(transform * skinMatrix)));

	normal = normalize(vectorTransform * in_normal);

	// Calculate fragment position
	vec4 fragPos = transform * skinMatrix * vec4(in_position, 1.0);
	
	// Pass values to fragment shader
	position = fragPos;
	textureCoord = in_textureCoord;

	// Calculate Tangent to object space matrix
	vec3 T = normalize(vectorTransform * in_tangent.xyz);
	vec3 N = normal;
	T = normalize(T - dot(T, N) * N); // re-orthogonalize T with respect to N
	vec3 B = normalize(cross(N, T) * in_tangent.w);
	TBN = mat3(T, B, N);

	tangent = T;

	// Set vertex position
	gl_Position = projection * view * fragPos;
}
