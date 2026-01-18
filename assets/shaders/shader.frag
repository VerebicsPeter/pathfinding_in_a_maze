#version 450 core
in vec2 fragUV;
out vec4 outColor;

layout(std430, binding = 0) buffer MyBuffer1 {
    int data_wall[];
};

layout(std430, binding = 1) buffer MyBuffer2 {
    int data_dist[];
};

layout(std430, binding = 2) buffer MyBuffer3 {
    int data_visit[];
};

uniform int imageWidth;
uniform int imageHeight;
uniform int maxDist;

void main() {
    int x = int(fragUV.x * imageWidth);
    int y = int(fragUV.y * imageHeight);
    int idx = y * imageWidth + x;

    if (idx >= imageWidth * imageHeight) discard;

    if (data_visit[idx] == 2) { // marked path
        outColor = vec4(0.0, 1.0, 0.0, 0.8);
        return;
    }

    if (data_wall[idx] < 0) { // wall
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    {
        // Normalize distance to [0, 1] range — adjust scale as needed
        float t = clamp(float(data_dist[idx]) / float(maxDist), 0.0, 1.0);

        vec3 colorA = vec3(0.2, 0.4, 1.0); // near
        vec3 colorB = vec3(1.0, 0.4, 0.2); // far

        vec3 mixed = mix(colorA, colorB, t);
        outColor = vec4(mixed, 0.5);
    }
}
