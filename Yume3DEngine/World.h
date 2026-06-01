#pragma once
#include <map>
#include <memory>
#include "Chunk.h"
#include "PerlinNoise.h"
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include "Structure.h"

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

struct ChunkPosHash {
    std::size_t operator()(const ChunkPos& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.z) << 1);
    }
};

class World {
public:
    std::map<ChunkPos, std::unique_ptr<Chunk>> chunks;

    // 核心大重构：用 vector 保持更新顺序，用 unordered_set 实现 O(1) 超级无敌快去重！
    std::vector<ChunkPos> terrainQueue;
    std::unordered_set<ChunkPos, ChunkPosHash> terrainSet;

    std::vector<ChunkPos> meshQueue;
    std::unordered_set<ChunkPos, ChunkPosHash> meshSet;

    //  O(1) 级别瞬间推入魔法！再也不用辛辛苦苦遍历整个 vector 了！
    void SafePushMeshQueue(const ChunkPos& pos) {
        if (meshSet.find(pos) == meshSet.end()) {
            meshQueue.push_back(pos);
            meshSet.insert(pos);
        }
    }

    void ForceUpdateInteractionMesh(const ChunkPos& pos) {
        if (chunks.count(pos) > 0) {
            UpdateChunkMesh(pos); // 轰！！！当场重绘，立刻上屏！

            // 魔法细节优化：既然这里已经当场硬熔炼完了，如果它原本在 meshQueue 队列里排队，
            // 我们就顺手把它抹掉，防止下一帧又白白重复熔炼一次，再次压榨 CPU！
            auto itM = std::find(meshQueue.begin(), meshQueue.end(), pos);
            if (itM != meshQueue.end()) {
                meshQueue.erase(itM);
            }
            meshSet.erase(pos); // 别忘了清理高速哈希阵
        }
    }

    void SafePushTerrainQueue(const ChunkPos& pos) {
        if (terrainSet.find(pos) == terrainSet.end()) {
            terrainQueue.push_back(pos);
            terrainSet.insert(pos);
        }
    }

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
        int currentChunkX = (int)floor(playerPos.x / 16.0f);
        int currentChunkZ = (int)floor(playerPos.z / 16.0f);

        // ====================================================
        // 【第一阶段：动态探测世界意图，老老实实写进排队单】
        // ====================================================
        for (int x = currentChunkX - radius; x <= currentChunkX + radius; x++) {
            for (int z = currentChunkZ - radius; z <= currentChunkZ + radius; z++) {
                ChunkPos pos = { x, z };
                if (chunks.count(pos) == 0) {
                    SafePushTerrainQueue(pos); // 瞬间判重并推入！
                }
            }
        }

        if (terrainQueue.size() > 1) {
            std::sort(terrainQueue.begin(), terrainQueue.end(), [currentChunkX, currentChunkZ](const ChunkPos& a, const ChunkPos& b) {
                int distA = std::abs(a.x - currentChunkX) + std::abs(a.z - currentChunkZ);
                int distB = std::abs(b.x - currentChunkX) + std::abs(b.z - currentChunkZ);
                return distA > distB; // 远的排在前面，近的排在尾部，完美拦截！
                });
        }

        // ====================================================
        // 【第二阶段：真·时空平摊双流消纳流水线（彻底干碎 glBufferData 死锁！）】
        // ====================================================

        //  流水线 A：每一帧【最多只允许凭空诞生 1 个】新区块的噪声方块肉体！
        // 这样一秒钟就能顺滑诞生 60 个区块，同时主线程的 CPU 耗时平稳得像心电图的直线一样！
        if (!terrainQueue.empty()) {
            ChunkPos posToCreate = terrainQueue.back();
            terrainQueue.pop_back();
            terrainSet.erase(posToCreate);

            if (chunks.count(posToCreate) == 0) {
                // (1) 让新区块的外壳在内存里诞生
                chunks[posToCreate] = std::make_unique<Chunk>();

                // (2) 调用纯净版的地形皮肤灌注，先把地表和坚石织出来
                FillFlatChunk(chunks[posToCreate].get(), posToCreate.x, posToCreate.z);

                // (3) 【正常且高级的无限播种降临】！
                // 用新区块的绝对坐标现编织一个局部伪随机种子
                unsigned int chunkSeed = static_cast<unsigned int>(posToCreate.x * 73129 + posToCreate.z * 95121);
                std::srand(chunkSeed);

                if ((std::rand() % 100) < 60) { // 保持相同的 60% 播种概率
                    int localX = std::rand() % 12 + 2; // 安全边界，严防越界崩溃！
                    int localZ = std::rand() % 12 + 2;

                    int globalX = posToCreate.x * 16 + localX;
                    int globalZ = posToCreate.z * 16 + localZ;

                    // 秒算地表高度
                    float tx = static_cast<float>(globalX);
                    float tz = static_cast<float>(globalZ);
                    float nBase = TerrainNoise::GetPerlin(tx * 0.025f, tz * 0.025f) * 25.0f;
                    float nDetail = TerrainNoise::GetPerlin(tx * 0.1f, tz * 0.1f) * 4.0f;
                    int surfH = static_cast<int>(40.0f + nBase + nDetail);

                    if (surfH < 1) surfH = 1;
                    if (surfH >= 256) surfH = 255;

                    // 轰！！！在新世界动态延展的瞬间，指挥独立的 Structure 在绝对坐标上把大树灌注出来！
                    Structure::GenerateTree(this, globalX, surfH + 1, globalZ);
                }

                // (4) 把自己推入 O(1) 熔炼队列，下一帧平滑重绘上屏
                SafePushMeshQueue(posToCreate);

                // 邻居区块连带重新熔炼（确保边界树叶和网格完美严丝合缝）
                ChunkPos neighbors[4] = {
                    {posToCreate.x + 1, posToCreate.z}, {posToCreate.x - 1, posToCreate.z},
                    {posToCreate.x, posToCreate.z + 1}, {posToCreate.x, posToCreate.z - 1}
                };
                for (const auto& nbPos : neighbors) {
                    if (chunks.count(nbPos) > 0) {
                        SafePushMeshQueue(nbPos);
                    }
                }
            }
        }

        if (meshQueue.size() > 1) {
            std::sort(meshQueue.begin(), meshQueue.end(), [currentChunkX, currentChunkZ](const ChunkPos& a, const ChunkPos& b) {
                int distA = std::abs(a.x - currentChunkX) + std::abs(a.z - currentChunkZ);
                int distB = std::abs(b.x - currentChunkX) + std::abs(b.z - currentChunkZ);
                return distA > distB;
                });
        }

        // ====================================================
        // 【第三阶段：冷酷卸载跑得太远的陈旧世界（保持你原汁原味的清理名单，防爆安全！）】
        // ====================================================
        if (!meshQueue.empty()) {
            ChunkPos posToUpdate = meshQueue.back();
            meshQueue.pop_back();
            meshSet.erase(posToUpdate); // 记得在哈希集合里也抹掉它

            if (chunks.count(posToUpdate) > 0) {
                UpdateChunkMesh(posToUpdate);
            }
        }

        // 4. 视距之外的区块驱逐魔法
        std::vector<ChunkPos> chunksToRemove;
        for (auto& pair : chunks) {
            ChunkPos pos = pair.first;
            if (abs(pos.x - currentChunkX) > radius + 1 || abs(pos.z - currentChunkZ) > radius + 1) {
                chunksToRemove.push_back(pos);
            }
        }
        for (ChunkPos pos : chunksToRemove) {
            chunks.erase(pos);

            // 这里的清除也全部同步进化为 O(1) 或者更安全的高效清除！
            auto itT = std::find(terrainQueue.begin(), terrainQueue.end(), pos);
            if (itT != terrainQueue.end()) terrainQueue.erase(itT);
            terrainSet.erase(pos);

            auto itM = std::find(meshQueue.begin(), meshQueue.end(), pos);
            if (itM != meshQueue.end()) meshQueue.erase(itM);
            meshSet.erase(pos);
        }
    }

    void GenerateWorld(int radius) {
        //  阶段一：让视距内所有的区块外壳，第一批在内存里整整齐齐地诞生！
        for (int x = -radius; x <= radius; x++) {
            for (int z = -radius; z <= radius; z++) {
                ChunkPos pos = { x, z };
                chunks[pos] = std::make_unique<Chunk>();
            }
        }

        //  阶段二：让所有区块各自独立灌注地形皮肤（草地、泥土、坚石）
        for (int x = -radius; x <= radius; x++) {
            for (int z = -radius; z <= radius; z++) {
                ChunkPos pos = { x, z };
                FillFlatChunk(chunks[pos].get(), x, z); // 纯净调用！
            }
        }

        //  阶段三：【高贵的正常生态降临】！此时因为四周的邻居区块全都有了内存外壳，
        // 我们在每个区块的 16x16 领空里用伪随机算好坐标，让独立的 Structure 去全自动种树！
        // 这样跨界伸出去的树叶就百分之百能在隔壁区块里生根发芽，绝对不漏气、不崩溃！
        for (int x = -radius; x <= radius; x++) {
            for (int z = -radius; z <= radius; z++) {

                unsigned int chunkSeed = static_cast<unsigned int>(x * 73129 + z * 95121);
                std::srand(chunkSeed);

                if ((std::rand() % 100) < 60) { // 60% 概率长树
                    int localX = std::rand() % 12 + 2; // 安全边界
                    int localZ = std::rand() % 12 + 2;

                    int globalX = x * 16 + localX;
                    int globalZ = z * 16 + localZ;

                    // 用你最爱的绝对全局坐标，秒算这一格精确的柏林噪声地表高度！
                    float tx = static_cast<float>(globalX);
                    float tz = static_cast<float>(globalZ);
                    float nBase = TerrainNoise::GetPerlin(tx * 0.025f, tz * 0.025f) * 25.0f;
                    float nDetail = TerrainNoise::GetPerlin(tx * 0.1f, tz * 0.1f) * 4.0f;
                    int surfH = static_cast<int>(40.0f + nBase + nDetail);

                    if (surfH < 1) surfH = 1;
                    if (surfH >= 256) surfH = 255;

                    // 跨越文件壁垒，指挥独立的 Structure 类在这一帧把树木当场在绝对坐标上灌注出来！
                    Structure::GenerateTree(this, globalX, surfH + 1, globalZ);
                }
            }
        }

        //  阶段四：最后，让整个装配完毕的 3D 世界进行唯一一次神盛的重绘熔炼，提交给显卡！
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