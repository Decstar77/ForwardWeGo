#version 330 core

in vec2 vTexCoords;

uniform vec3 uObjectColor;
uniform sampler2D uAlbedoTexture;
uniform bool uHasAlbedoTexture;
uniform bool uAllowTextures;

out vec4 FragColor;

void main() {
    vec3 albedo = uHasAlbedoTexture && uAllowTextures ? texture(uAlbedoTexture, vTexCoords).rgb * uObjectColor : uObjectColor;
    vec3 result = albedo * 0.9;
    FragColor = vec4(result, 1.0);
}
