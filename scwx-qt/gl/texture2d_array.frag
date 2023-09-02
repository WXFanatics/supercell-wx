#version 330 core

// Lower the default precision to medium
precision mediump float;

uniform sampler2DArray uTexture;

smooth in vec2 texCoord;
smooth in vec4 color;

layout (location = 0) out vec4 fragColor;

void main()
{
   fragColor = texture(uTexture, vec3(texCoord, 0.0)) * color;
}
