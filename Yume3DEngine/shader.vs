#version 330 core

layout(location = 2) in vec3 meshOrigin;   
layout(location = 3) in vec2 meshSize;     
layout(location = 4) in float blockID;     
layout(location = 5) in float faceID;      
// 梦子注意看！这里直接用一个 mat4 吃掉 6,7,8,9 四个通道！
layout(location = 6) in vec4 aoRow0;
layout(location = 7) in vec4 aoRow1;
layout(location = 8) in vec4 aoRow2;
layout(location = 9) in vec4 aoRow3;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 BlockUV;
flat out float TexIndex; 
flat out int FlatFace;

// 把阴影矩阵原封不动送往下一关！
flat out mat4 MicroAO;

void main() {
    int face = int(faceID);
    int triVert = gl_VertexID % 6; 

    int lookup[6] = int[](0, 2, 1, 0, 3, 2);
    int corner = lookup[triVert];

    float u = float((corner == 1) || (corner == 2));
    float v = float((corner == 2) || (corner == 3));

    int atlasLUT[18] = int[](
        1, 1, 1, 1, 2, 0, 
        2, 2, 2, 2, 2, 2, 
        3, 3, 3, 3, 3, 3
    );
    int bIndex = clamp(int(blockID) - 1, 0, 2);
    int lutIndex = bIndex * 6 + face;
    TexIndex = float(atlasLUT[lutIndex]);

    float sx = meshSize.x;
    float sy = meshSize.y;

    BlockUV = vec2(u * sx, v * sy);
    FlatFace = face;
    
    // 施放护盾！
    MicroAO = mat4(aoRow0, aoRow1, aoRow2, aoRow3);

    vec3 localPos = vec3(0.0);
    if (face == 0) localPos = vec3( u * sx,         v * sy, 0.0);
    if (face == 1) localPos = vec3((1.0 - u) * sx,  v * sy, 1.0);
    if (face == 2) localPos = vec3(0.0,             v * sy, (1.0 - u) * sx);
    if (face == 3) localPos = vec3(1.0,             v * sy, u * sx);
    if (face == 4) localPos = vec3( u * sx, 0.0,           (1.0 - v) * sy);
    if (face == 5) localPos = vec3( u * sx, 1.0,            v * sy);

    vec3 finalPos = localPos + meshOrigin;
    gl_Position = projection * view * model * vec4(finalPos, 1.0);
}