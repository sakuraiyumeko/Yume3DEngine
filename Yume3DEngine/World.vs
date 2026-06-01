#version 330 core

// 梦子注意：Location 2-6 完美挂接你 Chunk::Render 里的实例化通道！
layout(location = 2) in vec3 meshOrigin;   
layout(location = 3) in vec2 meshSize;
layout(location = 4) in float blockID;     
layout(location = 5) in float faceID;
layout(location = 6) in vec4 aoCorners;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 BlockUV;
flat out float TexIndex; 
out float InterpolatedAO;

void main() {
    int face = int(faceID);
    int triVert = gl_VertexID % 6;
    
    // 梦子神来之笔的 6 顶点查找表，本小姐帮你加上 const 让硬件疯狂加速！
    const int lookup[6] = int[](0, 2, 1, 0, 3, 2);
    int corner = lookup[triVert];

    // 硬件双线性插值完美分发
    if(corner == 0)      InterpolatedAO = aoCorners.x;
    else if(corner == 1) InterpolatedAO = aoCorners.y;
    else if(corner == 2) InterpolatedAO = aoCorners.z;
    else if(corner == 3) InterpolatedAO = aoCorners.w;

    float u = float((corner == 1) || (corner == 2));
    float v = float((corner == 2) || (corner == 3));

    float sx = meshSize.x;
    float sy = meshSize.y;
    BlockUV = vec2(u * sx, v * sy);

    //  扩展 LUT 魔法阵：为以后你那两个月心血来潮做的收音机和甜甜圈留下空位！
    // 假设以后有更多方块，我们将上限动态解开，或者在这里做好安全防御
    int atlasLUT[18] = int[](
        1, 1, 1, 1, 2, 0, // 草方块 (顶泥底)
        2, 2, 2, 2, 2, 2, // 泥土
        3, 3, 3, 3, 3, 3  // 石头
    );
    int bIndex = clamp(int(blockID) - 1, 0, 2); // 以后这里可以随心所欲扩展哦
    int lutIndex = bIndex * 6 + face;
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
