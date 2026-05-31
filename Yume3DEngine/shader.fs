#version 330 core
out vec4 FragColor;

// 从顶点着色器传过来的数据
in vec2 BlockUV;        // 这个是物理尺寸 UV，比如 (5.2, 3.8)
flat in float TexIndex; // 图集页码
flat in int FlatFace;   

uniform sampler2D texture1;

void main() {
    // --- 1. 核心修复：重新计算 atlasUV ---
    vec2 localUV = fract(BlockUV); // 每一格内部的 0.0-1.0
    
    // 梦子的图集是横向排列的（4格），所以 x 坐标要加上偏移再除以 4
    vec2 atlasUV = vec2((localUV.x + TexIndex) / 4.0, localUV.y);
    
    // 获取基础贴图颜色
    vec4 texColor = texture(texture1, atlasUV);

    // --- 2. 二维 AO 冲突解决方案 ---
    // 梦子，既然是二维合并，我们就不能用那个 16 长度的一维数组了
    // 这里我们采用一种“程序化伪 AO”逻辑，它能自动适应任何尺寸的矩形！
    
    float u = localUV.x;
    float v = localUV.y;

    // 算出当前像素点距离“每一小格”边缘的距离
    float distToEdgeX = min(u, 1.0 - u);
    float distToEdgeY = min(v, 1.0 - v);
    float distToEdge = min(distToEdgeX, distToEdgeY);

    // 只有在真正的“物理格点”边缘才产生阴影
    // smoothstep(边缘开始黑的地方, 边缘最黑的地方, 距离)
    float softShadow = smoothstep(0.0, 0.12, distToEdge);

    // --- 3. 最终光照计算 ---
    // 0.8 是基础环境光，0.2 是动态阴影贡献
    float ambient = 0.8 + 0.2 * softShadow;

    // 这样每一格方块之间都会有淡淡的凹陷感，而且绝对不会产生黑色条纹或崩溃！
    FragColor = vec4(texColor.rgb * ambient, texColor.a);
}