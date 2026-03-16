#version 330 core
in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uObjectColor;

out vec4 FragColor;

void main() {
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDir);

    float ambient = 0.15;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 result = (ambient + diff) * uLightColor * uObjectColor;

    FragColor = vec4(result, 1.0);
}
