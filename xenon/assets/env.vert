#version 460 core

layout(location = 0) in vec3 in_position;

out vec3 position;

uniform mat4 projection;
uniform mat4 view;

void main() {
	position = in_position;
	vec4 clipPos = projection * mat4(mat3(view)) * vec4(in_position, 1.0);

	gl_Position = clipPos.xyww;
}
