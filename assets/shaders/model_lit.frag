#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vTexCoords;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uObjectColor;
uniform sampler2D uAlbedoTexture;
uniform bool uHasAlbedoTexture;
uniform bool uAllowTextures;

out vec4 FragColor;

void main() {
    vec3 albedo = uHasAlbedoTexture && uAllowTextures ? texture(uAlbedoTexture, vTexCoords).rgb * uObjectColor : uObjectColor;

    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDir);

    float ambient = 0.15;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 result = (ambient + diff) * uLightColor * albedo;

    FragColor = vec4(result, 1.0);
}
