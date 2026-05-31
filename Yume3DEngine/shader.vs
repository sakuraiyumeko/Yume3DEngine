#version 330 core

layout(location = 2) in vec3 meshOrigin;   
layout(location = 3) in vec2 meshSize;
layout(location = 4) in float blockID;     
layout(location = 5) in float faceID;
// 真正的 location 6：接收 C++ 传过来的 vec4(左下, 右下, 右上, 左上)
layout(location = 6) in vec4 aoCorners;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 BlockUV;
flat out float TexIndex; 
// 统一变量名，和 fs 完美对接！
out float InterpolatedAO;

void main() {
    int face = int(faceID);
    int triVert = gl_VertexID % 6;
    
    int lookup[6] = int[](0, 2, 1, 0, 3, 2);
    int corner = lookup[triVert];

    // 硬件双线性插值分发
    if(corner == 0) InterpolatedAO = aoCorners.x;
    else if(corner == 1) InterpolatedAO = aoCorners.y;
    else if(corner == 2) InterpolatedAO = aoCorners.z;
    else if(corner == 3) InterpolatedAO = aoCorners.w;

    float u = float((corner == 1) || (corner == 2));
    float v = float((corner == 2) || (corner == 3));

    float sx = meshSize.x;
    float sy = meshSize.y;
    BlockUV = vec2(u * sx, v * sy);

    int atlasLUT[18] = int[](
        1, 1, 1, 1, 2, 0,
        2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3
    );
    int bIndex = clamp(int(blockID) - 1, 0, 2);
    int lutIndex = bIndex * 6 + face;
    TexIndex = float(atlasLUT[lutIndex]);

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