#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_tangent;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_textureCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 transform;

void main() {

	gl_Position = projection * view * transform * vec4(in_position, 1.0);

}
