#pragma once

#ifndef CHUNK_H
#define CHUNK_H

#include <glad/glad.h>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

enum class BlockType : uint8_t {
    AIR = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3
};

struct FaceInstance {
    glm::vec3 position;
    glm::vec2 size;
    float blockID;
    float faceID;
    // 完美的瘦身快递箱：只需要存 4 个角的 AO 值 (左下, 右下, 右上, 左上)
    glm::vec4 aoCorners;
};

static inline bool visitedPool[8192] = { false };

class Chunk {
public:
    BlockType blocks[16 * 256 * 16];
    unsigned int instanceVBO;
    int activeBlockCount;

    Chunk() : instanceVBO(0), activeBlockCount(0) {
        for (int i = 0; i < 16 * 256 * 16; i++) {
            blocks[i] = BlockType::AIR;
        }
        glGenBuffers(1, &instanceVBO);
    }

    ~Chunk() {
        if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
    }

    inline int getIndex(int x, int y, int z) const {
        return x * 4096 + y * 16 + z;
    }

    BlockType getBlockSafe(int x, int y, int z) const {
        if (x >= 0 && x < 16 && y >= 0 && y < 256 && z >= 0 && z < 16) {
            return blocks[getIndex(x, y, z)];
        }
        return BlockType::AIR;
    }

    void SetBlock(int x, int y, int z, BlockType type) {
        // 扣上安全大护盾，绝对不准往虚空里乱塞方块！
        if (x >= 0 && x < 16 && y >= 0 && y < 256 && z >= 0 && z < 16) {
            blocks[getIndex(x, y, z)] = type;
        }
    }

    // --- 终极魔法：带全局跨界检测的 AO 指纹计算 ---
    glm::vec4 CalculateSingleGridAO(int gx, int gy, int gz, int face, int chunkX, int chunkZ, const std::function<BlockType(int, int, int)>& getGlobalBlock) const {
        int worldX = chunkX * 16 + gx;
        int worldY = gy;
        int worldZ = chunkZ * 16 + gz;

        auto check = [&](int dx, int dy, int dz) -> int {
            return (getGlobalBlock(worldX + dx, worldY + dy, worldZ + dz) != BlockType::AIR) ? 1 : 0;
            };

        glm::ivec3 n = { 0, 0, 0 }; // 外侧法线方向
        glm::ivec3 u = { 0, 0, 0 }; // 局部水平 U 轴（必须严格对齐 shader.vs 里的 sx 拉伸轴！）
        glm::ivec3 v = { 0, 0, 0 }; // 局部纵向 V 轴（必须严格对齐 shader.vs 里的 sy 拉伸轴！）

        //  梦子看这里！真·六面空间正交手势绝对对齐名册！ 
        // 严格根据你在 shader.vs 里写的 localPos 镜像（1.0-u）与轴对调进行纯数学映射！
        if (face == 0) { n = { 0, 0, -1 }; u = { 1, 0, 0 };  v = { 0, 1, 0 }; } // 后：u=X, v=Y
        if (face == 1) { n = { 0, 0, 1 };  u = { -1, 0, 0 }; v = { 0, 1, 0 }; } // 前：u=(1.0-X), v=Y
        if (face == 2) { n = { -1, 0, 0 }; u = { 0, 0, -1 }; v = { 0, 1, 0 }; } // 左：u=(1.0-Z), v=Y 修正：v 才是高度 Y 轴！u 才是 Z 轴！
        if (face == 3) { n = { 1, 0, 0 };  u = { 0, 0, 1 };  v = { 0, 1, 0 }; } // 右：u=Z, v=Y        修正：v 才是高度 Y 轴！u 局限于 Z！
        if (face == 4) { n = { 0, -1, 0 }; u = { 1, 0, 0 };  v = { 0, 0, -1 }; } // 下：u=X, v=-Z
        if (face == 5) { n = { 0, 1, 0 };  u = { 1, 0, 0 };  v = { 0, 0, 1 }; } // 上：u=X, v=Z

        // 2. 万能四角邻居数格子闭包
        auto check_corner = [&](int du, int dv) -> int {
            int side1 = check(n.x + du * u.x, n.y + du * u.y, n.z + du * u.z);
            int side2 = check(n.x + dv * v.x, n.y + dv * v.y, n.z + dv * v.z);
            int corner = check(n.x + du * u.x + dv * v.x, n.y + du * u.y + dv * v.y, n.z + du * u.z + dv * v.z);

            return (side1 && side2) ? 3 : (side1 + side2 + corner);
            };

        // 3. 严格对齐你在着色器里的角点定义分发：0=左下(-1,-1), 1=右下(1,-1), 2=右上(1,1), 3=左上(-1,1)
        float c0 = 1.0f - float(check_corner(-1, -1)) * 0.18f;
        float c1 = 1.0f - float(check_corner(1, -1)) * 0.18f;
        float c2 = 1.0f - float(check_corner(1, 1)) * 0.18f;
        float c3 = 1.0f - float(check_corner(-1, 1)) * 0.18f;

        glm::vec4 finalAO = glm::vec4(c0, c1, c2, c3);
        for (int a = 0; a < 4; a++) {
            if (finalAO[a] < 0.45f) finalAO[a] = 0.45f;
            if (finalAO[a] > 1.0f)  finalAO[a] = 1.0f;
        }

        return finalAO;
    }

    void GenerateMesh(int chunkX, int chunkZ, const std::function<BlockType(int, int, int)>& getGlobalBlock) {
        std::vector<FaceInstance> validInstances;

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

                        // 【获取基准 AO 指纹】
                        glm::vec4 currentAO = CalculateSingleGridAO(coord[0], coord[1], coord[2], face, chunkX, chunkZ, getGlobalBlock);

                        // 贪婪拉伸 U 轴 (判定材质 + AO)
                        int width = 1;
                        while (u + width < dims[uAxis] && !visitedPool[u + width + v * dims[uAxis]]) {
                            int t[3] = { 0 }; t[axis] = i; t[uAxis] = u + width; t[vAxis] = v;
                            if (getBlockSafe(t[0], t[1], t[2]) != currentType) break;
                            if (CalculateSingleGridAO(t[0], t[1], t[2], face, chunkX, chunkZ, getGlobalBlock) != currentAO) break;
                            width++;
                        }

                        // 贪婪拉伸 V 轴 (判定材质 + AO)
                        int height = 1;
                        while (v + height < dims[vAxis]) {
                            bool canGrow = true;
                            for (int w = 0; w < width; w++) {
                                if (visitedPool[(u + w) + (v + height) * dims[uAxis]]) { canGrow = false; break; }
                                int t[3] = { 0 }; t[axis] = i; t[uAxis] = u + w; t[vAxis] = v + height;
                                if (getBlockSafe(t[0], t[1], t[2]) != currentType) { canGrow = false; break; }
                                if (CalculateSingleGridAO(t[0], t[1], t[2], face, chunkX, chunkZ, getGlobalBlock) != currentAO) { canGrow = false; break; }
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

    void Render(unsigned int VAO) {
        if (activeBlockCount == 0) return;
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        GLsizei stride = sizeof(FaceInstance);

        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)0); glVertexAttribDivisor(2, 1);
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3))); glVertexAttribDivisor(3, 1);
        glEnableVertexAttribArray(4); glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2))); glVertexAttribDivisor(4, 1);
        glEnableVertexAttribArray(5); glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float))); glVertexAttribDivisor(5, 1);

        // Location 6 接收完美的 aoCorners (vec4)
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float) * 2));
        glVertexAttribDivisor(6, 1);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, activeBlockCount);
        glBindVertexArray(0);
    }
};

#endif