#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

//  莉安酱为你定制的核心双模控制器！
uniform sampler2D screenTexture; // 纹理采样器
uniform int useTexture;          // 0 为纯色模式（准星/背景矩形），1 为贴图模式（快捷栏图标）
uniform vec4 uiColor;            // 外部传入的高贵颜色（包含 Alpha 半透明通道！）

void main()
{
    if (useTexture == 1)
    {
        // 贴图模式：读取材质原色，并乘上 uiColor 保持灵活性
        vec4 texColor = texture(screenTexture, TexCoords);
        
        // 极致的魔法细节防御：如果贴图里有透明像素（比如圆形图标的边缘），直接丢弃，防止露出恶心的黑边！
        if(texColor.a < 0.1)
            discard;
            
        FragColor = texColor * uiColor;
    }
    else
    {
        // 纯色模式：直接轰出梦子在 C++ 里指定的各种漂亮颜色！
        FragColor = uiColor;
    }
}
