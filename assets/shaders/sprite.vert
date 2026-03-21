#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

uniform vec2 uCenter;
uniform vec2 uHalfSize;

out vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = vec4(uCenter + aPos * uHalfSize, 0.0, 1.0);
}
