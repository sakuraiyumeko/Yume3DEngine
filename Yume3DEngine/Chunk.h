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

//  梦子高贵的单面实例化打包快递箱！
struct FaceInstance {
    glm::vec3 position;   // 巨型合并矩形的左下角起点坐标 (x, y, z)
    glm::vec2 size;       // 矩形跨越的物理格数 (Width, Height)
    float blockID;        // 材质密码 (1.0, 2.0...)
    float faceID;         // 面朝向标识 (0.0=后, 1.0=前, 2.0=左, 3.0=右, 4=下, 5=上)
    float microAO[16];
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

    //  辅助魔法：内部安全查询方块，越界一律回传空气
    BlockType getBlockSafe(int x, int y, int z) const {
        if (x >= 0 && x < 16 && y >= 0 && y < 256 && z >= 0 && z < 16) {
            return blocks[getIndex(x, y, z)];
        }
        return BlockType::AIR;
    }

    void SetBlock(int x, int y, int z, BlockType type) {
        if (x >= 0 && x < 16 && y >= 0 && y < 256 && z >= 0 && z < 16) {
            blocks[getIndex(x, y, z)] = type;
        }
    }

    // 终极禁咒：全自动化六面贪婪面合并熔炼术！ 
    void GenerateMesh(int chunkX, int chunkZ, const std::function<BlockType(int, int, int)>& getGlobalBlock) {
        std::vector<FaceInstance> validInstances;

        for (int face = 0; face < 6; face++) {
            // 判定当前朝向是属于哪个轴的切片 (0,1 -> Z轴; 2,3 -> X轴; 4,5 -> Y轴)
            int axis = (face == 0 || face == 1) ? 2 : ((face == 2 || face == 3) ? 0 : 1);

            // 确定另外两个用来建立 2D 扫描网格的平面轴 (如 Z 轴切片，则在 X-Y 平面上扫描)
            int uAxis = (axis == 0) ? 1 : 0;
            int vAxis = (axis == 2) ? 1 : 2;

            int dims[3] = { 16, 256, 16 }; // 三维最大边界限制
            int currentPlaneSize = dims[uAxis] * dims[vAxis];

            // 顺着主切片轴一层层切下去
            for (int i = 0; i < dims[axis]; i++) {
                // 零开销机制清空大缓存池
                std::fill(visitedPool, visitedPool + currentPlaneSize, false);

                // 开始在 2D 平面上用扫描线拉伸矩形
                for (int v = 0; v < dims[vAxis]; v++) {
                    for (int u = 0; u < dims[uAxis]; u++) {
                        int visitedIdx = u + v * dims[uAxis];
                        if (visitedPool[visitedIdx]) continue;

                        //  核心修复一：用绝对规矩的 [3] 明确声明标准的 3D 整数局部数组！ 
                        if (visitedIdx < 0 || visitedIdx >= 8192) continue;
                        if (visitedPool[visitedIdx]) continue;

                        int coord[3] = { 0, 0, 0 };
                        coord[axis] = i; coord[uAxis] = u; coord[vAxis] = v;

                        BlockType current = getBlockSafe(coord[0], coord[1], coord[2]);
                        if (current == BlockType::AIR) continue;

                        // 跨区块跨界判定：只有这个面真正见到了空气，才允许加入合并大家庭！
                        bool isFaceVisible = false;
                        int nx = coord[0], ny = coord[1], nz = coord[2];
                        if (face == 0) nz--; if (face == 1) nz++;
                        if (face == 2) nx--; if (face == 3) nx++;
                        if (face == 4) ny--; if (face == 5) ny++;

                        if (nx < 0 || nx >= 16 || nz < 0 || nz >= 16) {
                            if (getGlobalBlock(chunkX * 16 + nx, ny, chunkZ * 16 + nz) == BlockType::AIR) isFaceVisible = true;
                        }
                        else {
                            if (getBlockSafe(nx, ny, nz) == BlockType::AIR) isFaceVisible = true;
                        }

                        if (!isFaceVisible) { visitedPool[visitedIdx] = true; continue; }

                        //  贪婪拉伸一：横向能拉多宽就拉多宽（U轴拉伸）
                        int width = 1;
                        while (u + width < dims[uAxis] && !visitedPool[u + width + v * dims[uAxis]]) {
                            //  核心修复二：把这里的所有 testCoord 全都对齐标准 [3] 数组！ 
                            int testCoord[3] = { 0, 0, 0 };
                            testCoord[axis] = i; testCoord[uAxis] = u + width; testCoord[vAxis] = v;

                            if (getBlockSafe(testCoord[0], testCoord[1], testCoord[2]) != current) break;

                            int nnx = testCoord[0], nny = testCoord[1], nnz = testCoord[2];
                            if (face == 0) nnz--; if (face == 1) nnz++;
                            if (face == 2) nnx--; if (face == 3) nnx++;
                            if (face == 4) nny--; if (face == 5) nny++;

                            bool nFaceVis = false;
                            if (nnx < 0 || nnx >= 16 || nnz < 0 || nnz >= 16) {
                                if (getGlobalBlock(chunkX * 16 + nnx, nny, chunkZ * 16 + nnz) == BlockType::AIR) nFaceVis = true;
                            }
                            else {
                                if (getBlockSafe(nnx, nny, nnz) == BlockType::AIR) nFaceVis = true;
                            }

                            if (!nFaceVis) break;
                            width++;
                        }

                        //  贪婪拉伸二：纵向能推多高就推多高（V轴整体向上平移）
                        int height = 1;
                        bool canGrowV = true;
                        while (v + height < dims[vAxis] && canGrowV) {
                            for (int w = 0; w < width; w++) {
                                if (visitedPool[(u + w) + (v + height) * dims[uAxis]]) { canGrowV = false; break; }

                                //  核心修复三：把这里的 testCoord 同样对齐标准 [3] 数组！ 
                                int testCoord[3] = { 0, 0, 0 };
                                testCoord[axis] = i; testCoord[uAxis] = u + w; testCoord[vAxis] = v + height;

                                if (getBlockSafe(testCoord[0], testCoord[1], testCoord[2]) != current) { canGrowV = false; break; }

                                int nnnx = testCoord[0], nnny = testCoord[1], nnnz = testCoord[2];
                                if (face == 0) nnnz--; if (face == 1) nnnz++;
                                if (face == 2) nnnx--; if (face == 3) nnnx++;
                                if (face == 4) nnny--; if (face == 5) nnny++;

                                bool nnnFaceVis = false;
                                if (nnnx < 0 || nnnx >= 16 || nnnz < 0 || nnnz >= 16) {
                                    if (getGlobalBlock(chunkX * 16 + nnnx, nnny, chunkZ * 16 + nnnz) == BlockType::AIR) nnnFaceVis = true;
                                }
                                else {
                                    if (getBlockSafe(nnnx, nnny, nnnz) == BlockType::AIR) nnnFaceVis = true;
                                }

                                if (!nnnFaceVis) { canGrowV = false; break; }
                            }
                            if (canGrowV) height++;
                        }

                        // 矩形网格拉伸完毕，打包 FaceInstance 属性快递箱
                        FaceInstance inst;
                        //  核心修复四：把绝对起点 startCoord 同样对齐标准 [3] 数组！ 
                        int startCoord[3] = { 0, 0, 0 };
                        startCoord[axis] = i; startCoord[uAxis] = u; startCoord[vAxis] = v;
                        inst.position = glm::vec3((float)startCoord[0], (float)startCoord[1], (float)startCoord[2]);

                        if (axis == 2)      inst.size = glm::vec2((float)width, (float)height);
                        else if (axis == 0) inst.size = glm::vec2((float)height, (float)width);
                        else                inst.size = glm::vec2((float)width, (float)height);

                        inst.blockID = static_cast<float>(current);
                        inst.faceID = static_cast<float>(face);

                        // 初始化你高贵的 16 格微观阴影天眼数组
                        for (int a = 0; a < 16; a++) {
                            inst.microAO[a] = 1.0f;
                        }

                        int maxAOLen = (width < 16) ? width : 16;
                        for (int w_idx = 0; w_idx < maxAOLen; w_idx++) {
                            int curGridCoord[3] = { 0, 0, 0 };
                            curGridCoord[axis] = i;
                            curGridCoord[uAxis] = u + w_idx;
                            curGridCoord[vAxis] = v;

                            if (face == 5) {
                                int gx = curGridCoord[0];
                                int gy = curGridCoord[1];
                                int gz = curGridCoord[2];

                                int side1 = (getBlockSafe(gx - 1, gy + 1, gz) != BlockType::AIR) ? 1 : 0;
                                int side2 = (getBlockSafe(gx, gy + 1, gz - 1) != BlockType::AIR) ? 1 : 0;
                                int side3 = (getBlockSafe(gx + 1, gy + 1, gz) != BlockType::AIR) ? 1 : 0;
                                int side4 = (getBlockSafe(gx, gy + 1, gz + 1) != BlockType::AIR) ? 1 : 0;

                                int corner1 = (getBlockSafe(gx - 1, gy + 1, gz - 1) != BlockType::AIR) ? 1 : 0;
                                int corner2 = (getBlockSafe(gx + 1, gy + 1, gz - 1) != BlockType::AIR) ? 1 : 0;
                                int corner3 = (getBlockSafe(gx + 1, gy + 1, gz + 1) != BlockType::AIR) ? 1 : 0;
                                int corner4 = (getBlockSafe(gx - 1, gy + 1, gz + 1) != BlockType::AIR) ? 1 : 0;

                                int totalBlockades = side1 + side2 + side3 + side4 + corner1 + corner2 + corner3 + corner4;

                                if (totalBlockades > 0) {
                                    float rawAO = 1.0f - (float)totalBlockades * 0.15f;
                                    inst.microAO[w_idx] = (rawAO < 0.45f) ? 0.45f : ((rawAO > 1.0f) ? 1.0f : rawAO);
                                }
                            }
                        }
                        // 打上 visited 标记，防止重复融炼
                        for (int h = 0; h < height; h++) {
                            for (int w = 0; w < width; w++) {
                                visitedPool[(u + w) + (v + h) * dims[uAxis]] = true;
                            }
                        }

                        validInstances.push_back(inst);
                    }
                }
            }
        }

        activeBlockCount = static_cast<int>(validInstances.size());
        if (activeBlockCount > 0) {
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            // 梦子！这里一定要用 .data()，并且大小要算对哦！
            glBufferData(GL_ARRAY_BUFFER, validInstances.size() * sizeof(FaceInstance), validInstances.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    void Render(unsigned int VAO) {
        if (activeBlockCount == 0) return;

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        // 计算步长：这就是每个实例在内存里占据的字节总数
        GLsizei stride = sizeof(FaceInstance);

        // ... 前面的 2, 3, 4, 5 通道保持不变 ...
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glVertexAttribDivisor(2, 1);

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));
        glVertexAttribDivisor(3, 1);

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
        glVertexAttribDivisor(4, 1);

        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(float)));
        glVertexAttribDivisor(5, 1);

        size_t offsetAO = offsetof(FaceInstance, microAO);
        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(6 + i); // 占用 6, 7, 8, 9 四个位置
            glVertexAttribPointer(
                6 + i,
                4, GL_FLOAT,
                GL_FALSE,
                stride,
                (void*)(offsetAO + i * sizeof(float) * 4)
            );
            glVertexAttribDivisor(6 + i, 1); // 记得是实例化属性！
        }

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, activeBlockCount);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

#endif