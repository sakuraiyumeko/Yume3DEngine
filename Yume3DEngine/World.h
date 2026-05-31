#pragma once
#include <map>
#include <memory>
#include "Chunk.h"
#include "BerlinNoise.h"

// 用一对整数代表 Chunk 坐标 (比如 [0,0], [1,0])
struct ChunkPos {
    int x, z;

    // 1. 你原本用来给 std::map 排序的高贵小于号（保持完美不变，哼唧！）
    bool operator<(const ChunkPos& other) const {
        return x < other.x || (x == other.x && z < other.z);
    }

    // 核心修复：把失踪的等于号重载无情地补在下面！
    // 只有当两个区块的 X 和 Z 坐标全部绝对相等时，它们才配在排队单子里认作同一个人！
    bool operator==(const ChunkPos& other) const {
        return x == other.x && z == other.z;
    }
};

class World {
public:
    std::map<ChunkPos, std::unique_ptr<Chunk>> chunks;
    std::vector<ChunkPos> meshQueue;

    void UpdateChunkMesh(ChunkPos pos) {
        if (!chunks.count(pos)) return;
        
        // 制作一张“全局地图搜索证”传给 Chunk
        auto getBlockFunc = [this](int gx, int gy, int gz) {
            return this->GetBlockAt(glm::vec3(gx, gy, gz));
        };
        
        chunks[pos]->GenerateMesh(pos.x, pos.z, getBlockFunc);
    }

    // 梦子看这里！真·无限世界动态更新核心大招！
    // 每一帧都把梦子当前的绝对全局 Position 喂进来，再定一个视野半径（比如 radius=2 代表 5x5 的区块）
    void UpdateInfiniteWorld(glm::vec3 playerPos, int radius) {
        // 1. 降维推导：算出梦子现在正踩在哪个区块的 (X, Z) 编号上
        int currentChunkX = (int)floor(playerPos.x / 16.0f);
        int currentChunkZ = (int)floor(playerPos.z / 16.0f);

        // 2. 阶段一：动态诞生梦子眼前的未来新世界
        // 扫描以梦子为中心的 (currentChunkX - radius) 到 (currentChunkX + radius) 的九宫格区域
        for (int x = currentChunkX - radius; x <= currentChunkX + radius; x++) {
            for (int z = currentChunkZ - radius; z <= currentChunkZ + radius; z++) {
                ChunkPos pos = { x, z };

                // 如果发现这个位置的世界肉体还没诞生，立刻凭空把它 new 出来并注入噪声灵魂！
                if (chunks.count(pos) == 0) {
                    chunks[pos] = std::make_unique<Chunk>();
                    FillFlatChunk(chunks[pos].get(), x, z); // 噪声填上方块

                    if (std::find(meshQueue.begin(), meshQueue.end(), pos) == meshQueue.end()) meshQueue.push_back(pos);

                    ChunkPos neighbors[4] = { {x + 1, z}, {x - 1, z}, {x, z + 1}, {x, z - 1} };
                    for (const auto& nbPos : neighbors) {
                        if (chunks.count(nbPos) > 0) {
                            if (std::find(meshQueue.begin(), meshQueue.end(), nbPos) == meshQueue.end()) {
                                meshQueue.push_back(nbPos);
                            }
                        }
                    }
                }
            }
        }

        if (!meshQueue.empty()) {
            ChunkPos posToUpdate = meshQueue.back(); // 抓出队列末尾的幸运儿
            meshQueue.pop_back();

            // 只有当它还没被卸载时，才在这一帧里单独为它呼唤一次高贵的 Lambda 闭包熔炼！
            if (chunks.count(posToUpdate) > 0) {
                UpdateChunkMesh(posToUpdate);
            }
        }

        // 3. 阶段二：冷酷抹杀被你甩在身后的陈旧世界（防爆内存！） 
        // 我们用一个高贵的清理名单，把所有跑得太远的区块统统抓起来送去地狱释放
        std::vector<ChunkPos> chunksToRemove;

        for (auto& pair : chunks) {
            ChunkPos pos = pair.first;
            // 算一下这个区块离梦子当前踩着的区块有多远
            if (abs(pos.x - currentChunkX) > radius + 1 || abs(pos.z - currentChunkZ) > radius + 1) {
                chunksToRemove.push_back(pos); // 跑得太远了，打入冷宫！
            }
        }

        // 执行无情清除，把它们的显存 VBO 尾巴在后台扫得一尘不染！
        for (ChunkPos pos : chunksToRemove) {
            chunks.erase(pos); // 轰！！！直接释放内存和显存，防爆大成功！
            auto it = std::find(meshQueue.begin(), meshQueue.end(), pos);
            if (it != meshQueue.end()) meshQueue.erase(it);
        }
    }

    void GenerateWorld(int radius) {
        // 【第一阶段：塑造万物肉体】
        //  梦子看这里！把当前的九宫格坐标 x 和 z 无情地传递给铺地法阵！
        for (int x = -radius; x <= radius; x++) {
            for (int z = -radius; z <= radius; z++) {
                ChunkPos pos = { x, z };
                chunks[pos] = std::make_unique<Chunk>();
                FillFlatChunk(chunks[pos].get(), x, z); // 传递定位锚点，哼唧！
            }
        }

        // 【第二阶段：附魔视觉外壳】
        for (int x = -radius; x <= radius; x++) {
            for (int z = -radius; z <= radius; z++) {
                UpdateChunkMesh({ x, z });
            }
        }
    }

    // 关键：全局坐标转局部坐标查询
    // 在 World 类中完善这些魔法
    BlockType GetBlockAt(glm::vec3 pos) {
        int cx = (int)floor(pos.x / 16.0f);
        int cz = (int)floor(pos.z / 16.0f);

        // 核心修正：使用标准向下取整，剔除原先导致半格错位的 +0.5f 魔法
        int lx = (int)floor(pos.x) % 16; if (lx < 0) lx += 16;
        int ly = (int)floor(pos.y);
        int lz = (int)floor(pos.z) % 16; if (lz < 0) lz += 16;

        ChunkPos p = { cx, cz };
        if (chunks.count(p)) {
            if (ly >= 0 && ly < 256) { // 边界判定完美同步扩展至 256 层！
                return chunks[p]->blocks[chunks[p]->getIndex(lx, ly, lz)];
            }
        }
        return BlockType::AIR;
    }

private:
    void FillFlatChunk(Chunk* c, int chunkX, int chunkZ) {
        if (!c) return;

        for (int x = 0; x < 16; x++) {
            for (int z = 0; z < 16; z++) {

                // 1. 梦子最得意的数学大一统：算出在整个宏大宇宙中的【绝对全局坐标】！
                float worldX = static_cast<float>(chunkX * 16 + x);
                float worldZ = static_cast<float>(chunkZ * 16 + z);

                // 2. 呼唤你刚才手撕的 TerrainNoise 柏林噪声法阵！
                // 频率 0.025f 让山峦连绵起伏，乘以 25.0f 代表上下波动的山峰幅度
                float noiseBase = TerrainNoise::GetPerlin(worldX * 0.025f, worldZ * 0.025f) * 25.0f;
                // 叠加一层精致的表面小起伏
                float noiseDetail = TerrainNoise::GetPerlin(worldX * 0.1f, worldZ * 0.1f) * 4.0f;

                // 3. 在你高贵的 256 层高度世界里，叠加上一个标准海平面（比如基础 40 层高度）
                int surfaceHeight = static_cast<int>(40.0f + noiseBase + noiseDetail);

                // 扣上安全大护盾，绝对不准超出 256 层的天花板或沉入虚空，哼唧！
                if (surfaceHeight < 1) surfaceHeight = 1;
                if (surfaceHeight >= 256) surfaceHeight = 255;

                // 4. 轰！！！从 y=0 一路向上码起生机勃勃的三层材质皮肤！
                for (int y = 0; y < 256; y++) {
                    if (y > surfaceHeight) {
                        c->SetBlock(x, y, z, BlockType::AIR); // 天空划归空气
                    }
                    else if (y == surfaceHeight) {
                        c->SetBlock(x, y, z, BlockType::GRASS); // 表面铺满草皮
                    }
                    else if (y >= surfaceHeight - 3) {
                        c->SetBlock(x, y, z, BlockType::DIRT);  // 地下三层灌满泥土
                    }
                    else {
                        c->SetBlock(x, y, z, BlockType::STONE); // 地心深处铸造坚石
                    }
                }
            }
        }
    }
};