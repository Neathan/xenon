#version 460 core

//---------------------------------------------------------------
// [SECTION] Input data & output variables
//---------------------------------------------------------------

in vec2 textureCoord;

in vec3 nearPoint;
in vec3 farPoint;

in flat mat4 fragView;
in flat mat4 fragProjection;

layout(location = 0) out vec4 fragColor;


uniform float near;
uniform float far;


//---------------------------------------------------------------
// [SECTION] Grid rendering function
//---------------------------------------------------------------

vec4 grid(vec3 fragPos, float scale) {
	vec2 coord = fragPos.xz * scale;
	vec2 derivative = fwidth(coord);
	vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
	float line = min(grid.x, grid.y);
	float minimumz = min(derivative.y, 1);
	float minimumx = min(derivative.x, 1);
	vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
	// X-axis
	if(fragPos.z > -0.1 * minimumz && fragPos.z < 0.1 * minimumz) {
		color.r = 1.0;
	}
	// Z-axis
	if(fragPos.x > -0.1 * minimumx && fragPos.x < 0.1 * minimumx) {
		color.b = 1.0;
	}
	return color;
}


//---------------------------------------------------------------
// [SECTION] Depth calculation
//---------------------------------------------------------------

float computeDepth(vec3 pos) {
	float far = gl_DepthRange.far;
	float near = gl_DepthRange.near;

	vec4 clipSpacePos = fragProjection * fragView * vec4(pos, 1.0);
	float clipSpaceDepth = (clipSpacePos.z / clipSpacePos.w);
	return (((far-near) * clipSpaceDepth) + near + far) / 2.0;
}


//---------------------------------------------------------------
// [SECTION] Main program
//---------------------------------------------------------------

void main() {
	float t = -nearPoint.y / (farPoint.y - nearPoint.y);
	vec3 fragPos = nearPoint + t * (farPoint - nearPoint);


	float depth = computeDepth(fragPos);
	float dist = (length((fragView * vec4(fragPos, 1)).xyz) + near);
	float fading = min(1.0, 1 - dist * 5 / far);

	fragColor = grid(fragPos, 1) * float(t > 0);
	fragColor.a *= fading;

	gl_FragDepth = depth;
}

