#version 460 core

out vec4 fragColor;

in vec2 textureCoord;

layout(binding = 0) uniform sampler2D framebufferTexture;


void main(void) {
	fragColor = texture(framebufferTexture, textureCoord);
}
