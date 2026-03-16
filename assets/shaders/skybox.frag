#version 330 core
in vec3 vWorldDir;

uniform sampler2D uEquirectMap;

out vec4 FragColor;

const float PI = 3.14159265359;

void main() {
    vec3 dir = normalize(vWorldDir);
    float phi = atan(dir.z, dir.x);
    float theta = asin(clamp(dir.y, -1.0, 1.0));
    vec2 uv = vec2(phi / (2.0 * PI) + 0.5, theta / PI + 0.5);
    FragColor = texture(uEquirectMap, uv);
}
