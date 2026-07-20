#version 330 core
out vec4 FragColor;

in vec2 BlockUV;
in float InterpolatedAO; 
in float InterpolatedLight;
flat in float TexIndex; 

uniform sampler2D texture1;

void main() {
    // 依旧是用 fract 拿取微观像素面内部的相对平铺坐标
    vec2 localUV = fract(BlockUV);
    
    // 【魔法修复：绝对边缘护盾锁】
    // 为了严防死守 Mipmap 或者是插值精度丢失导致去蹭到隔壁方块的材质，
    // 我们用一个极其微小的像素缩进（比如 0.005f 到 0.001f），强行把 localUV 约束在 [0.001, 0.999] 的绝对领域内！
    vec2 clampedUV = clamp(localUV, 0.001, 0.999);
    
    float totalTiles = 16.0; 
    
    // 使用被魔法锁死保护好的 clampedUV 算绝对大图集坐标，漏气 Bug 瞬间烟消云散，哼唧！
    vec2 atlasUV = vec2((clampedUV.x + TexIndex) / totalTiles, clampedUV.y);
    vec4 texColor = texture(texture1, atlasUV);
    
    // 如果梦子你以后想做透明树叶镂空效果，本小姐在这里顺手赐予你一发“虚无驱逐咒”！
    if (texColor.a < 0.1) {
        discard; // 如果是纯透明像素，当场轰飞，绝不渲染！
    }
    
    // 环境光遮蔽 (AO)
    float ambient = InterpolatedAO * 0.92; 

    // 计算非线性体素光照衰减，还原高贵纯正的 Voxel 明暗风骨！
    float lightFactor = pow(0.86, 15.0 - InterpolatedLight);
    float lightIntensity = max(lightFactor, 0.12); 

    FragColor = vec4(texColor.rgb * ambient * lightIntensity, texColor.a);
}
