#version 330 core
in vec2 vUV;

uniform float uAlpha;

out vec4 fragColor;

void main() {
    vec2 uv = vUV * 2.0 - 1.0;  // Remap to -1..1
    float dist = length(uv);

    // Smooth vignette: transparent in center, opaque at edges
    float vignette = smoothstep(0.3, 2.0, dist);

    fragColor = vec4(0.6, 0.0, 0.0, vignette * uAlpha);
}
