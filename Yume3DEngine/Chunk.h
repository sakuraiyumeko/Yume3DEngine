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
    STONE = 3,
    LOG = 4,
    LEAVES = 5,
    GLOWSTONE = 6
};

struct FaceInstance {
    glm::vec3 position;
    glm::vec2 size;
    float blockID;
    float faceID;
    glm::vec4 aoCorners; // (左下, 右下, 右上, 左上) 完美的四个角环境光遮蔽
    float faceLight;
    float faceTexIndex;
};

static inline bool visitedPool[8192] = { false };

class Chunk {
public:
    BlockType blocks[16 * 256 * 16];
    uint8_t light[16 * 256 * 16];
    unsigned int instanceVBO;
    int activeBlockCount;

    Chunk() : instanceVBO(0), activeBlockCount(0) {
        for (int i = 0; i < 16 * 256 * 16; i++) {
            blocks[i] = BlockType::AIR;
            light[i] = 0;
        }
        glGenBuffers(1, &instanceVBO);
    }

    ~Chunk() {
        if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
    }

    inline int getIndex(int x, int y, int z) const {
        return x * 4096 + y * 16 + z;
    }

    inline uint8_t GetLight(int x, int y, int z) const {
        if (x >= 0 && x < 16 && y >= 0 && y < 256 && z >= 0 && z < 16) {
            return light[getIndex(x, y, z)];
        }
        return 0;
    }

    inline void SetLight(int x, int y, int z, uint8_t val) {
        if (x >= 0 && x < 16 && y >= 0 && y < 256 && z >= 0 && z < 16) {
            light[getIndex(x, y, z)] = val;
        }
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

    template<typename Func>
    // --- 终极魔法：带全局跨界检测的 AO 指纹计算 ---
    glm::vec4 CalculateSingleGridAO(int gx, int gy, int gz, int face, int chunkX, int chunkZ, Func getGlobalBlock) const {
        // 1. 算出在宏观宇宙里的绝对绝对方块坐标，跨区块阴影完美相融！
        int worldX = chunkX * 16 + gx;
        int worldY = gy;
        int worldZ = chunkZ * 16 + gz;

        //  核心优化一：我们只保留这单单一个最纯粹、绝对没有二级套娃的最底层世界查询闭包！
        auto check = [&](int dx, int dy, int dz) -> int {
            return (getGlobalBlock(worldX + dx, worldY + dy, worldZ + dz) != BlockType::AIR) ? 1 : 0;
            };

        //  核心优化二：真·全主轴 6 面 24 方向常数位移全量大展开！ 
        // 我们不在最里面用向量乘法打架了！在进门的一瞬间，直接根据 face 把 4 个端点各自周围贴着的 3 个邻居相对绝对位移在内存里排好队！
        // 顺序雷打不动严格对齐：c0(左下), c1(右下), c2(右上), c3(左上) 
        int s0_1, s0_2, c0_n; // 角0的三个方向 (side1, side2, corner)
        int s1_1, s1_2, c1_n; // 角1的三个方向
        int s2_1, s2_2, c2_n; // 角2的三个方向
        int s3_1, s3_2, c3_n; // 角3的三个方向

        if (face == 0) { // 后面：看向 -Z。外侧是 z-1。局部 u=X, v=Y
            s0_1 = check(-1, 0, -1); s0_2 = check(0, -1, -1); c0_n = check(-1, -1, -1);
            s1_1 = check(1, 0, -1);  s1_2 = check(0, -1, -1); c1_n = check(1, -1, -1);
            s2_1 = check(1, 0, -1);  s2_2 = check(0, 1, -1);  c2_n = check(1, 1, -1);
            s3_1 = check(-1, 0, -1); s3_2 = check(0, 1, -1);  c3_n = check(-1, 1, -1);
        }
        else if (face == 1) { // 前面：看向 +Z。外侧是 z+1。局部 u=-X, v=Y
            s0_1 = check(1, 0, 1);  s0_2 = check(0, -1, 1); c0_n = check(1, -1, 1);
            s1_1 = check(-1, 0, 1); s1_2 = check(0, -1, 1); c1_n = check(-1, -1, 1);
            s2_1 = check(-1, 0, 1); s2_2 = check(0, 1, 1);  c2_n = check(-1, 1, 1);
            s3_1 = check(1, 0, 1);  s3_2 = check(0, 1, 1);  c3_n = check(1, 1, 1);
        }
        else if (face == 2) { // 左面：看向 -X。外侧是 x-1。局部 u=-Z, v=Y
            s0_1 = check(-1, 0, 1);  s0_2 = check(-1, -1, 0); c0_n = check(-1, -1, 1);
            s1_1 = check(-1, 0, -1); s1_2 = check(-1, -1, 0); c1_n = check(-1, -1, -1);
            s2_1 = check(-1, 0, -1); s2_2 = check(-1, 1, 0);  c2_n = check(-1, 1, -1);
            s3_1 = check(-1, 0, 1);  s3_2 = check(-1, 1, 0);  c3_n = check(-1, 1, 1);
        }
        else if (face == 3) { // 右面：看向 +X。外侧是 x+1。局部 u=Z, v=Y
            s0_1 = check(1, 0, -1); s0_2 = check(1, -1, 0); c0_n = check(1, -1, -1);
            s1_1 = check(1, 0, 1);  s1_2 = check(1, -1, 0); c1_n = check(1, -1, 1);
            s2_1 = check(1, 0, 1);  s2_2 = check(1, 1, 0);  c2_n = check(1, 1, 1);
            s3_1 = check(1, 0, -1); s3_2 = check(1, 1, 0);  c3_n = check(1, 1, -1);
        }
        else if (face == 4) { // 底面：看向 -Y。外侧是 y-1。局部 u=X, v=-Z
            s0_1 = check(-1, -1, 0); s0_2 = check(0, -1, 1); c0_n = check(-1, -1, 1);
            s1_1 = check(1, -1, 0);  s1_2 = check(0, -1, 1); c1_n = check(1, -1, 1);
            s2_1 = check(1, -1, 0);  s2_2 = check(0, -1, -1); c2_n = check(1, -1, -1);
            s3_1 = check(-1, -1, 0); s3_2 = check(0, -1, -1); c3_n = check(-1, -1, -1);
        }
        else { // 顶面：看向 +Y。外侧是 y+1。局部 u=X, v=Z (完美对齐你原本最自豪的初版公式！)
            s0_1 = check(-1, 1, 0); s0_2 = check(0, 1, -1); c0_n = check(-1, 1, -1);
            s1_1 = check(1, 1, 0);  s1_2 = check(0, 1, -1); c1_n = check(1, 1, -1);
            s2_1 = check(1, 1, 0);  s2_2 = check(0, 1, 1);  c2_n = check(1, 1, 1);
            s3_1 = check(-1, 1, 0); s3_2 = check(0, 1, 1);  c3_n = check(-1, 1, 1);
        }

        //  4. 纯数学免死变暗公式（全自动剔除平原，只有墙角才触发！）
        float v0 = (s0_1 && s0_2) ? 0.45f : 1.0f - (s0_1 + s0_2 + c0_n) * 0.18f;
        float v1 = (s1_1 && s1_2) ? 0.45f : 1.0f - (s1_1 + s1_2 + c1_n) * 0.18f;
        float v2 = (s2_1 && s2_2) ? 0.45f : 1.0f - (s2_1 + s2_2 + c2_n) * 0.18f;
        float v3 = (s3_1 && s3_2) ? 0.45f : 1.0f - (s3_1 + s3_2 + c3_n) * 0.18f;

        // 5. 区间限幅锁
        glm::vec4 finalAO = glm::vec4(v0, v1, v2, v3);
        for (int a = 0; a < 4; a++) {
            if (finalAO[a] < 0.45f) finalAO[a] = 0.45f;
            if (finalAO[a] > 1.0f)  finalAO[a] = 1.0f;
        }

        return finalAO;
    }

    template<typename BlockFunc, typename LightFunc>
    void GenerateMesh(int chunkX, int chunkZ, BlockFunc getGlobalBlock, LightFunc getGlobalLight) {
        std::vector<FaceInstance> validInstances;

        auto getFaceLight = [&](int lx, int ly, int lz, int f) -> float {
            int nx = lx, ny = ly, nz = lz;
            if (f == 0) nz--; else if (f == 1) nz++;
            else if (f == 2) nx--; else if (f == 3) nx++;
            else if (f == 4) ny--; else if (f == 5) ny++;
            uint8_t lightVal = getGlobalLight(chunkX * 16 + nx, ny, chunkZ * 16 + nz);
            return (float)std::max((lightVal >> 4) & 0xF, lightVal & 0xF);
            };

        for (int face = 0; face < 6; face++) {
            int axis = (face == 0 || face == 1) ? 2 : ((face == 2 || face == 3) ? 0 : 1);
            int uAxis = (axis == 0) ? 1 : 0;
            int vAxis = (axis == 2) ? 1 : 2;
            int dims[3] = { 16, 256, 16 };

            // 建立微型局部属性冷冻表（最大 16x256，小巧得可以直接塞进 CPU 缓存行！）
            // 专门存储当前扫描面中：[u + v * dims[uAxis]] 的状态
            struct GridProp { glm::vec4 ao; float light; bool isAir; };
            static GridProp propCache[16 * 256];

            for (int i = 0; i < dims[axis]; i++) {
                // 【魔法优化 1】精准擦除当前层大小，绝不乱蹭无辜内存
                int uDim = dims[uAxis];
                int vDim = dims[vAxis];
                std::fill(visitedPool, visitedPool + (uDim * vDim), false);

                // 【核心大招二：属性批量预烘焙】在进门前，把这一层所有格子的 AO 和光照一次性算完！
                for (int v = 0; v < vDim; v++) {
                    for (int u = 0; u < uDim; u++) {
                        int idx = u + v * uDim;
                        int coord[3] = { 0, 0, 0 };
                        coord[axis] = i; coord[uAxis] = u; coord[vAxis] = v;

                        BlockType currentType = getBlockSafe(coord[0], coord[1], coord[2]);
                        if (currentType == BlockType::AIR) {
                            propCache[idx].isAir = true;
                            continue;
                        }

                        // 预先判明可见性
                        int nx = coord[0], ny = coord[1], nz = coord[2];
                        if (face == 0) nz--; else if (face == 1) nz++;
                        else if (face == 2) nx--; else if (face == 3) nx++;
                        else if (face == 4) ny--; else if (face == 5) ny++;

                        bool isVisible = false;
                        if (nx < 0 || nx >= 16 || nz < 0 || nz >= 16) {
                            if (getGlobalBlock(chunkX * 16 + nx, ny, chunkZ * 16 + nz) == BlockType::AIR) isVisible = true;
                        }
                        else {
                            if (getBlockSafe(nx, ny, nz) == BlockType::AIR) isVisible = true;
                        }

                        if (!isVisible) {
                            propCache[idx].isAir = true; // 视作不可见，直接跳过
                        }
                        else {
                            propCache[idx].isAir = false;
                            // 这里高贵的 CalculateSingleGridAO 已经被换成了零开销模版版本，哼唧！
                            propCache[idx].ao = CalculateSingleGridAO(coord[0], coord[1], coord[2], face, chunkX, chunkZ, getGlobalBlock);
                            propCache[idx].light = getFaceLight(coord[0], coord[1], coord[2], face);
                        }
                    }
                }

                // 【第二阶段：极速合并】现在里面的 while 循环只需要查表，没有任何多余计算！
                for (int v = 0; v < vDim; v++) {
                    for (int u = 0; u < uDim; u++) {
                        int visitedIdx = u + v * uDim;
                        if (visitedPool[visitedIdx] || propCache[visitedIdx].isAir) continue;

                        int coord[3] = { 0, 0, 0 };
                        coord[axis] = i; coord[uAxis] = u; coord[vAxis] = v;
                        BlockType currentType = getBlockSafe(coord[0], coord[1], coord[2]);

                        glm::vec4 currentAO = propCache[visitedIdx].ao;
                        float currentLight = propCache[visitedIdx].light;

                        // 贪婪 U 轴：瞬间查表！
                        int width = 1;
                        while (u + width < uDim && !visitedPool[u + width + v * uDim]) {
                            int nextIdx = u + width + v * uDim;
                            if (propCache[nextIdx].isAir) break;
                            int t[3] = { 0 }; t[axis] = i; t[uAxis] = u + width; t[vAxis] = v;
                            if (getBlockSafe(t[0], t[1], t[2]) != currentType) break;
                            if (propCache[nextIdx].ao != currentAO || propCache[nextIdx].light != currentLight) break;
                            width++;
                        }

                        // 贪婪 V 轴：瞬间查表！
                        int height = 1;
                        while (v + height < vDim) {
                            bool canGrow = true;
                            for (int w = 0; w < width; w++) {
                                int nextIdx = (u + w) + (v + height) * uDim;
                                if (visitedPool[nextIdx] || propCache[nextIdx].isAir) { canGrow = false; break; }
                                int t[3] = { 0 }; t[axis] = i; t[uAxis] = u + w; t[vAxis] = v + height;
                                if (getBlockSafe(t[0], t[1], t[2]) != currentType) { canGrow = false; break; }
                                if (propCache[nextIdx].ao != currentAO || propCache[nextIdx].light != currentLight) { canGrow = false; break; }
                            }
                            if (!canGrow) break;
                            height++;
                        }

                        // 装配上屏数据
                        FaceInstance inst;
                        int start[3] = { 0 }; start[axis] = i; start[uAxis] = u; start[vAxis] = v;
                        inst.position = glm::vec3((float)start[0], (float)start[1], (float)start[2]);
                        inst.size = (axis == 0) ? glm::vec2((float)height, (float)width) : glm::vec2((float)width, (float)height);
                        inst.blockID = (float)currentType;
                        inst.faceID = (float)face;
                        inst.aoCorners = currentAO;
                        inst.faceLight = currentLight;

                        validInstances.push_back(inst);

                        // 标记脚印
                        for (int hv = 0; hv < height; hv++)
                            for (int uw = 0; uw < width; uw++)
                                visitedPool[(u + uw) + (v + hv) * uDim] = true;
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

        // Location 6 为 aoCorners (vec4)
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float) * 2));
        glVertexAttribDivisor(6, 1);

        // Location 7 为 faceLight (float)
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float) * 2 + sizeof(glm::vec4)));
        glVertexAttribDivisor(7, 1);

        // 紧跟在 Location 7 的后面追加这一句，哼唧~
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float) * 2 + sizeof(glm::vec4) + sizeof(float)));
        glVertexAttribDivisor(8, 1);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, activeBlockCount);
        glBindVertexArray(0);
    }
};

#endif