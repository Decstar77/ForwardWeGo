#version 330 core
in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
uniform bool uHasTexture;

out vec4 FragColor;

void main() {
    vec4 texColor = uHasTexture ? texture(uTexture, vUV) : vec4(1.0);
    FragColor = texColor * vColor;
    if (FragColor.a < 0.01) {
        discard;
    }
}
