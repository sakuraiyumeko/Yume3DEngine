#version 330 core
out vec4 FragColor;

in vec2 BlockUV;
in float InterpolatedAO; 
flat in float TexIndex; // 接收来自 VS 的高贵贴图索引

uniform sampler2D texture1;

void main() {
    vec2 localUV = fract(BlockUV);
    
    //  终极数学大修正：把原本写死的 4.0，狠狠地改成你现在大长条图里【实际的小图总格数】！
    // 比如：0层底泥, 1层侧草, 2层顶草, 3层石头, 4层原木顶, 5层原木侧, 6层原木底, 7层橡树叶...
    // 梦子数一数你的长条图里一共有几格，假设一共有 9 格，这里就无脑改成 9.0 ！
    float totalTiles = 16; 
    
    // 严丝合缝的横向图集 UV 映射公式，再也不会把材质压扁啦！
    vec2 atlasUV = vec2((localUV.x + TexIndex) / totalTiles, localUV.y);
    vec4 texColor = texture(texture1, atlasUV);
    
    // 最终环境光 = 顶点插值的 AO * 基础环境亮度系数 0.92
    float ambient = InterpolatedAO * 0.92; 

    FragColor = vec4(texColor.rgb * ambient, texColor.a);
}
