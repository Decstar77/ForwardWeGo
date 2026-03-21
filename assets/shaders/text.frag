#version 330 core
in vec2 vUV;

uniform sampler2D uFontAtlas;
uniform vec4 uColor;

out vec4 fragColor;

void main() {
    // stbtt UVs with opengl_fillrule=0 map directly to GL texture coords
    // (bitmap row 0 → GL texel y=0 → v=0, matching stbtt t=0)
    float alpha = texture(uFontAtlas, vUV).r;
    fragColor = vec4(uColor.rgb, uColor.a * alpha);
    //fragColor = vec4(1,1,1,1);
}
