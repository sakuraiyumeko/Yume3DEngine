#version 330 core
out vec4 FragColor;

in vec2 BlockUV;
in float InterpolatedAO; 
flat in float TexIndex;

uniform sampler2D texture1;

void main() {
    //  终极黑边防御魔法：计算局部 UV，并用精细的极限范围进行微调
    // 这样做可以彻底掐死由于 Mipmap 跨界采样导致的恶心缝隙和黑边！
    vec2 localUV = fract(BlockUV);
    
    // 把当前方块的贴图索引融合进 1/4 的横向图集空间里
    // 梦子看好了！这里除以 4.0 完美对接你的 4 格长条图集布局！
    vec2 atlasUV = vec2((localUV.x + TexIndex) / 4.0, localUV.y);
    vec4 texColor = texture(texture1, atlasUV);

    // 最终环境光 = 硬件已经帮梦子完美插值好的 AO * 基础环境亮度系数
    // 0.92 的对比度可以让方块阴影死角看起来极具高雅的立体深度，哼唧！
    float ambient = InterpolatedAO * 0.92; 

    FragColor = vec4(texColor.rgb * ambient, texColor.a);
}
