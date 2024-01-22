#version 330 core
in vec2 TexCoords;
in vec4 particleColor;
out vec4 color;

uniform sampler2D tex;

void main()
{
    color = texture(tex, TexCoords) * vec4(particleColor);
}