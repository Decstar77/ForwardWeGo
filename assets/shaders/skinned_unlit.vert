#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aBoneWeights;

const int MAX_BONES = 128;

uniform mat4 uViewProjection;
uniform mat4 uModel;
uniform mat4 uBoneMatrices[MAX_BONES];

out vec3 vNormal;
out vec3 vFragPos;
out vec2 vTexCoords;

void main() {
    mat4 boneTransform = mat4(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 4; i++) {
        if (aBoneIDs[i] >= 0) {
            boneTransform += uBoneMatrices[aBoneIDs[i]] * aBoneWeights[i];
            totalWeight += aBoneWeights[i];
        }
    }
    if (totalWeight > 0.0) {
        boneTransform /= totalWeight;
    } else {
        boneTransform = mat4(1.0);
    }

    vec4 skinnedPos = boneTransform * vec4(aPos, 1.0);
    vec4 worldPos = uModel * skinnedPos;
    vFragPos = worldPos.xyz;
    vTexCoords = aTexCoords;

    vec3 skinnedNormal = mat3(boneTransform) * aNormal;
    vNormal = mat3(transpose(inverse(uModel))) * skinnedNormal;

    gl_Position = uViewProjection * worldPos;
}
