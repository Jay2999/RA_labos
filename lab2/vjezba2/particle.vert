#version 330 core
layout (location = 0) in vec4 vertex;

out vec2 TexCoords;
out vec4 particleColor;

uniform mat4 projection;
uniform mat4 camOrientation;
uniform mat4 model;
uniform mat4 view;
uniform vec3 offset;
uniform vec4 color;
uniform float scale;

void main()
{
    TexCoords = vertex.zw;
    particleColor = color;
    vec4 particlePos = model * vec4((vec3(vertex.xy, 0) * scale) + offset, 1.0);

    gl_Position = projection * view * camOrientation * model * vec4((vec3(vertex.xy, 0) * scale) + offset, 1.0);
}