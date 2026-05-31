// shader.fs
#version 330 core
out vec4 FragColor;

in vec2 BlockUV;
in float InterpolatedAO; // 硬件已经帮梦子插值好了，超丝滑！
flat in float TexIndex;

uniform sampler2D texture1;

void main() {
    vec2 localUV = fract(BlockUV);
    vec2 atlasUV = vec2((localUV.x + TexIndex) / 4.0, localUV.y);
    vec4 texColor = texture(texture1, atlasUV);

    // 每一格的微小凹槽感（可选，如果不想要格子感可以删掉这一段）
    float edge = smoothstep(0.0, 0.08, min(localUV.x, 1.0-localUV.x)) * smoothstep(0.0, 0.08, min(localUV.y, 1.0-localUV.y));
    
    // 最终环境光 = 顶点插值的 AO * 格子边缘微调
    float ambient = InterpolatedAO * (0.92 + 0.08 * edge);

    FragColor = vec4(texColor.rgb * ambient, texColor.a);
}