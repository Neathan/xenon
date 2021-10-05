#version 460 core

layout(location = 0) in vec3 in_position;
layout(location = 3) in vec2 in_textureCoord;

out vec2 textureCoord;

void main() {
	textureCoord = in_textureCoord;
	gl_Position = vec4(in_position, 1.0);

}
