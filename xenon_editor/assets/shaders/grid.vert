#version 460 core

//---------------------------------------------------------------
// [SECTION] Input data & output variables
//---------------------------------------------------------------

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_tangent;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_textureCoord;

out vec2 textureCoord;

out vec3 nearPoint;
out vec3 farPoint;

out flat mat4 fragView;
out flat mat4 fragProjection;

//---------------------------------------------------------------
// [SECTION] Matricies
//---------------------------------------------------------------

uniform mat4 projection;
uniform mat4 view;


vec3 unprojectPoint(float x, float y, float z) {
	mat4 viewInv = inverse(view);
	mat4 projInv = inverse(projection);
	vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
	return unprojectedPoint.xyz / unprojectedPoint.w;
}

//---------------------------------------------------------------
// [SECTION] Main program
//---------------------------------------------------------------

void main() {
	vec3 p = in_position;
	nearPoint = unprojectPoint(p.x, p.y, 0.0).xyz; // unprojecting on the near plane
	farPoint = unprojectPoint(p.x, p.y, 1.0).xyz; // unprojecting on the far plane
	gl_Position = vec4(p, 1.0); // using directly the clipped coordinates

	textureCoord = in_textureCoord;
	fragView = view;
	fragProjection = projection;
}

