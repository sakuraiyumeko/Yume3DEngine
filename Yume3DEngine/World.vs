#version 330 core

// 梦子注意：Location 2-7 完美挂接实例化通道！
layout(location = 2) in vec3 meshOrigin;   
layout(location = 3) in vec2 meshSize;
layout(location = 4) in float blockID;     
layout(location = 5) in float faceID;
layout(location = 6) in vec4 aoCorners;
layout(location = 7) in float faceLight;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 BlockUV;
flat out float TexIndex; 
out float InterpolatedAO;
out float InterpolatedLight;

// 【魔法修复 1】把沉重的查找表无情踢出 main 函数，并用 const 锁死在显卡只读魔力池中！
const int atlasLUT[36] = int[](
    1, 1, 1, 1, 2, 0, 
    2, 2, 2, 2, 2, 2, 
    3, 3, 3, 3, 3, 3, 
    5, 6, 5, 6, 4, 7, 
    8, 8, 8, 8, 8, 8, 
    3, 3, 3, 3, 3, 3  
);

// 6 顶点查找表也稳稳安置在外面
const int lookup[6] = int[](0, 2, 1, 0, 3, 2);

void main() {
    int face = int(faceID);
    int triVert = gl_VertexID % 6;
    int corner = lookup[triVert];

    // 【魔法修复 2】硬件双线性插值
    if(corner == 0)      InterpolatedAO = aoCorners.x;
    else if(corner == 1) InterpolatedAO = aoCorners.y;
    else if(corner == 2) InterpolatedAO = aoCorners.z;
    else if(corner == 3) InterpolatedAO = aoCorners.w;

    // 【资深旅行家的画质秘籍】
    // 梦子，如果以后你发现拉长的大表面对角线阴影切痕很丑，本小姐允许你以后在片元着色器（Fragment Shader）里
    // 用重心坐标或者 UV 比例来手工做非线性 AO 混合，但在这里我们先用常量化让它快到飞起，哼唧！
    
    InterpolatedLight = faceLight;

    float u = float((corner == 1) || (corner == 2));
    float v = float((corner == 2) || (corner == 3));

    float sx = meshSize.x;
    float sy = meshSize.y;
    BlockUV = vec2(u * sx, v * sy);

    // 绝对安全的查表索引计算
    int bIndex = clamp(int(blockID) - 1, 0, 5);
    int lutIndex = bIndex * 6 + face;
    if (lutIndex < 0 || lutIndex >= 36) {
        lutIndex = 12; 
    }

    TexIndex = float(atlasLUT[lutIndex]);

    // 3D 实例化空间坐标构建
    vec3 localPos = vec3(0.0);
    if (face == 0) localPos = vec3( u * sx,          v * sy, 0.0);
    if (face == 1) localPos = vec3((1.0 - u) * sx,   v * sy, 1.0);
    if (face == 2) localPos = vec3(0.0,              v * sy, (1.0 - u) * sx);
    if (face == 3) localPos = vec3(1.0,              v * sy, u * sx);
    if (face == 4) localPos = vec3( u * sx, 0.0,            (1.0 - v) * sy);
    if (face == 5) localPos = vec3( u * sx, 1.0,             v * sy);

    vec3 finalPos = localPos + meshOrigin;
    gl_Position = projection * view * model * vec4(finalPos, 1.0);
}
