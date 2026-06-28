#include "Chunk.h"
#include <glad/glad.h>
#include "PerlinNoise.h" //  轰！！！在这里包含噪声魔导书，时空死锁瞬间被彻底掐死！
#include <algorithm>
#include <cmath>

// 1. 实现 24方向 AO 算法
glm::vec4 Chunk::CalculateSingleGridAO(int gx, int gy, int gz, int face, int chunkX, int chunkZ, const std::function<BlockType(int, int, int)>& getGlobalBlock) const {
    int worldX = chunkX * 16 + gx;
    int worldY = gy;
    int worldZ = chunkZ * 16 + gz;

    auto check = [&](int dx, int dy, int dz) -> int {
        return (getGlobalBlock(worldX + dx, worldY + dy, worldZ + dz) != BlockType::AIR) ? 1 : 0;
        };

    int s0_1, s0_2, c0_n; int s1_1, s1_2, c1_n; int s2_1, s2_2, c2_n; int s3_1, s3_2, c3_n;
    if (face == 0) {
        s0_1 = check(-1, 0, -1); s0_2 = check(0, -1, -1); c0_n = check(-1, -1, -1);
        s1_1 = check(1, 0, -1);  s1_2 = check(0, -1, -1); c1_n = check(1, -1, -1);
        s2_1 = check(1, 0, -1);  s2_2 = check(0, 1, -1);  c2_n = check(1, 1, -1);
        s3_1 = check(-1, 0, -1); s3_2 = check(0, 1, -1);  c3_n = check(-1, 1, -1);
    }
    else if (face == 1) {
        s0_1 = check(1, 0, 1);  s0_2 = check(0, -1, 1); c0_n = check(1, -1, 1);
        s1_1 = check(-1, 0, 1); s1_2 = check(0, -1, 1); c1_n = check(-1, -1, 1);
        s2_1 = check(-1, 0, 1); s2_2 = check(0, 1, 1);  c2_n = check(-1, 1, 1);
        s3_1 = check(1, 0, 1);  s3_2 = check(0, 1, 1);  c3_n = check(1, 1, 1);
    }
    else if (face == 2) {
        s0_1 = check(-1, 0, 1);  s0_2 = check(-1, -1, 0); c0_n = check(-1, -1, 1);
        s1_1 = check(-1, 0, -1); s1_2 = check(-1, -1, 0); c1_n = check(-1, -1, -1);
        s2_1 = check(-1, 0, -1); s2_2 = check(-1, 1, 0);  c2_n = check(-1, 1, -1);
        s3_1 = check(-1, 0, 1);  s3_2 = check(-1, 1, 0);  c3_n = check(-1, 1, 1);
    }
    else if (face == 3) {
        s0_1 = check(1, 0, -1); s0_2 = check(1, -1, 0); c0_n = check(1, -1, -1);
        s1_1 = check(1, 0, 1);  s1_2 = check(1, -1, 0); c1_n = check(1, -1, 1);
        s2_1 = check(1, 0, 1);  s2_2 = check(1, 1, 0);  c2_n = check(1, 1, 1);
        s3_1 = check(1, 0, -1); s3_2 = check(1, 1, 0);  c3_n = check(1, 1, -1);
    }
    else if (face == 4) {
        s0_1 = check(-1, -1, 0); s0_2 = check(0, -1, 1); c0_n = check(-1, -1, 1);
        s1_1 = check(1, -1, 0);  s1_2 = check(0, -1, 1); c1_n = check(1, -1, 1);
        s2_1 = check(1, -1, 0);  s2_2 = check(0, -1, -1); c2_n = check(1, -1, -1);
        s3_1 = check(-1, -1, 0); s3_2 = check(0, -1, -1); c3_n = check(-1, -1, -1);
    }
    else {
        s0_1 = check(-1, 1, 0); s0_2 = check(0, 1, -1); c0_n = check(-1, 1, -1);
        s1_1 = check(1, 1, 0);  s1_2 = check(0, 1, -1); c1_n = check(1, 1, -1);
        s2_1 = check(1, 1, 0);  s2_2 = check(0, 1, 1);  c2_n = check(1, 1, 1);
        s3_1 = check(-1, 1, 0); s3_2 = check(0, 1, 1);  c3_n = check(-1, 1, 1);
    }

    float v0 = (s0_1 && s0_2) ? 0.45f : 1.0f - (s0_1 + s0_2 + c0_n) * 0.18f;
    float v1 = (s1_1 && s1_2) ? 0.45f : 1.0f - (s1_1 + s1_2 + c1_n) * 0.18f;
    float v2 = (s2_1 && s2_2) ? 0.45f : 1.0f - (s2_1 + s2_2 + c2_n) * 0.18f;
    float v3 = (s3_1 && s3_2) ? 0.45f : 1.0f - (s3_1 + s3_2 + c3_n) * 0.18f;

    glm::vec4 finalAO = glm::vec4(v0, v1, v2, v3);
    for (int a = 0; a < 4; a++) {
        if (finalAO[a] < 0.45f) finalAO[a] = 0.45f;
        if (finalAO[a] > 1.0f)  finalAO[a] = 1.0f;
    }
    return finalAO;
}

// =========================================================================
//  2. 真·像素级常数空间懒人光照前置贪婪面合并核心算法
// =========================================================================
void Chunk::GenerateMesh(int chunkX, int chunkZ, const std::function<BlockType(int, int, int)>& getGlobalBlock) {
    std::vector<FaceInstance> validInstances;

    int localMaxY[16][16];
    for (int lx = 0; lx < 16; lx++) {
        for (int lz = 0; lz < 16; lz++) {
            int h = 255;
            while (h > 0 && getBlockSafe(lx, h, lz) == BlockType::AIR) {
                h--;
            }
            localMaxY[lx][lz] = h;
        }
    }

    auto getSpaceLightLazy = [&](int tx, int ty, int tz, int currentFace) -> float {
        int bx = tx; int by = ty; int bz = tz;
        if (currentFace == 0) bz--; else if (currentFace == 1) bz++;
        else if (currentFace == 2) bx--; else if (currentFace == 3) bx++;
        else if (currentFace == 4) by--; else if (currentFace == 5) by++;

        if (bx >= 0 && bx < 16 && by >= 0 && by < 256 && bz >= 0 && bz < 16) {
            // 区块内部直接 O(1) 抽取内存中的 skyLight ??！
            return static_cast<float>(blocks[getIndex(bx, by, bz)].skyLight);
        }
        else {
            // ? 跨区块边界判定：通过高贵的 getGlobalBlock 转换绝对坐标，
            // 只要高度在 40 层海平面以上，无条件给予 15 级阳光保底防越界变黑！
            return (by >= 40) ? 15.0f : 0.0f;
        }
    };

    for (int face = 0; face < 6; face++) {
        int axis = (face == 0 || face == 1) ? 2 : ((face == 2 || face == 3) ? 0 : 1);
        int uAxis = (axis == 0) ? 1 : 0;
        int vAxis = (axis == 2) ? 1 : 2;
        int dims[3] = { 16, 256, 16 };

        for (int i = 0; i < dims[axis]; i++) {
            std::fill(visitedPool, visitedPool + (dims[uAxis] * dims[vAxis]), false);

            for (int v = 0; v < dims[vAxis]; v++) {
                for (int u = 0; u < dims[uAxis]; u++) {
                    int visitedIdx = u + v * dims[uAxis];
                    if (visitedPool[visitedIdx]) continue;

                    int coord[3] = { 0, 0, 0 };
                    coord[axis] = i; coord[uAxis] = u; coord[vAxis] = v;

                    BlockType currentType = getBlockSafe(coord[0], coord[1], coord[2]);
                    if (currentType == BlockType::AIR) continue;

                    bool isVisible = false;
                    int nx = coord[0], ny = coord[1], nz = coord[2];
                    if (face == 0) nz--; else if (face == 1) nz++;
                    else if (face == 2) nx--; else if (face == 3) nx++;
                    else if (face == 4) ny--; else if (face == 5) ny++;

                    if (nx < 0 || nx >= 16 || nz < 0 || nz >= 16) {
                        if (getGlobalBlock(chunkX * 16 + nx, ny, chunkZ * 16 + nz) == BlockType::AIR) isVisible = true;
                    }
                    else {
                        if (getBlockSafe(nx, ny, nz) == BlockType::AIR) isVisible = true;
                    }
                    if (!isVisible) { visitedPool[visitedIdx] = true; continue; }

                    glm::vec4 currentAO = CalculateSingleGridAO(coord[0], coord[1], coord[2], face, chunkX, chunkZ, getGlobalBlock);

                    float currentLightValue = getSpaceLightLazy(coord[0], coord[1], coord[2], face);

                    int width = 1;
                    while (u + width < dims[uAxis] && !visitedPool[u + width + v * dims[uAxis]]) {
                        int t[3] = { 0 }; t[axis] = i; t[uAxis] = u + width; t[vAxis] = v;
                        if (getBlockSafe(t[0], t[1], t[2]) != currentType) break;
                        if (CalculateSingleGridAO(t[0], t[1], t[2], face, chunkX, chunkZ, getGlobalBlock) != currentAO) break;
                        if (getSpaceLightLazy(t[0], t[1], t[2], face) != currentLightValue) break;
                        width++;
                    }

                    int height = 1;
                    while (v + height < dims[vAxis]) {
                        bool canGrow = true;
                        for (int w = 0; w < width; w++) {
                            if (visitedPool[(u + w) + (v + height) * dims[uAxis]]) { canGrow = false; break; }
                            int t[3] = { 0 }; t[axis] = i; t[uAxis] = u + w; t[vAxis] = v + height;
                            if (getBlockSafe(t[0], t[1], t[2]) != currentType) { canGrow = false; break; }
                            if (CalculateSingleGridAO(t[0], t[1], t[2], face, chunkX, chunkZ, getGlobalBlock) != currentAO) { canGrow = false; break; }
                            if (getSpaceLightLazy(t[0], t[1], t[2], face) != currentLightValue) { canGrow = false; break; }
                        }
                        if (!canGrow) break;
                        height++;
                    }

                    FaceInstance inst;
                    int start[3] = { 0 }; start[axis] = i; start[uAxis] = u; start[vAxis] = v;
                    inst.position = glm::vec3((float)start[0], (float)start[1], (float)start[2]);

                    if (axis == 2)      inst.size = glm::vec2((float)width, (float)height);
                    else if (axis == 0) inst.size = glm::vec2((float)height, (float)width);
                    else                inst.size = glm::vec2((float)width, (float)height);

                    inst.blockID = (float)currentType;
                    inst.faceID = (float)face;
                    inst.lightData = glm::vec2(currentLightValue, 0.0f);
                    inst.aoCorners = currentAO;

                    validInstances.push_back(inst);
                    for (int hv = 0; hv < height; hv++)
                        for (int uw = 0; uw < width; uw++)
                            visitedPool[(u + uw) + (v + hv) * dims[uAxis]] = true;
                }
            }
        }
    }

    activeBlockCount = (int)validInstances.size();
    if (activeBlockCount > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, validInstances.size() * sizeof(FaceInstance), validInstances.data(), GL_STATIC_DRAW);
    }
}

void Chunk::Render(unsigned int VAO) {
    if (activeBlockCount == 0) return;
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    GLsizei stride = sizeof(FaceInstance);

    // Location 2: position (vec3) -> 偏移量 0
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glVertexAttribDivisor(2, 1);

    // Location 3: size (vec2) -> 偏移量 vec3 = 12 字节
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));
    glVertexAttribDivisor(3, 1);

    // Location 4: blockID (float) -> 偏移量 vec3 + vec2 = 20 字节
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
    glVertexAttribDivisor(4, 1);

    // Location 5: faceID (float) -> 偏移量 vec3 + vec2 + float = 24 字节
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float)));
    glVertexAttribDivisor(5, 1);

    //  Location 6：接收全新的光照数据 lightData (vec2) 
    // 偏移量精密计算：vec3 + vec2 + float + float = 前面一共占了 28 字节！也就是 sizeof(float) * 7 的空间！
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float) * 2));
    glVertexAttribDivisor(6, 1);

    //  Location 7：aoCorners 被顶到通道 7，位置完美错开！
    // 偏移量精密计算：前面所有的东西加上 lightData(vec2) 一共占了 28 + 8 = 36 字节！也就是 sizeof(float) * 9 的空间！
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float) * 4));
    glVertexAttribDivisor(7, 1);

    // 挥下画笔，把这个区块内部的所有实例方块一口气画出来！
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, activeBlockCount);
    glBindVertexArray(0);
}