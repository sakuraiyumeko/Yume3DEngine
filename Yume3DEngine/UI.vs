#version 330 core

// 梦子快看！完美对接 UIRenderer 通道 0 的位置和通道 1 的 UV 坐标
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

// 接收来自 UIRenderer 的神圣平移缩放矩阵（画准星时传入的是单位矩阵，直接白嫖！）
uniform mat4 model;

void main()
{
    // 把 2D 的坐标完美转换成现代 OpenGL 认识的 4D 裁剪空间坐标
    gl_Position = model * vec4(aPos, 0.0, 1.0);
    
    // 把高贵的纹理坐标原封不动地派发给片段着色器
    TexCoords = aTexCoords;
}
