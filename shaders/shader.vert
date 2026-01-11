#version 450 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

out vec2 fragUV;

uniform mat4 vp;
uniform int imageWidth;
uniform int imageHeight;

void main() {
    fragUV = inUV;
    vec2 worldPos = inPos * vec2(float(imageWidth), float(imageHeight));
    gl_Position = vp * vec4(worldPos, 0.0, 1.0);
}
