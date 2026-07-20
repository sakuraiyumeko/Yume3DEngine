#pragma once
#include <map>
#include <memory>
#include "Chunk.h"
#include "PerlinNoise.h"
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include "Structure.h"
#include <queue>

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

// 【核心新增 1】要把这个负责搬运“亮光”的快递小哥放在类外哦！
struct LightNode {
    int x, y, z;
};

// 【核心新增 2】要把这个专门负责在黑暗中清除“魔力余晖”的幽灵小哥也放在类外！
struct LightRemovalNode {
    int x, y, z;
    uint8_t val; // 记录被抹黑前的最后一丝光芒强度，哼唧~
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

    bool IsTransparent(BlockType type) {
        return type == BlockType::AIR || type == BlockType::LEAVES;
    }

    uint8_t GetLightAt(int gx, int gy, int gz) {
        if (gy < 0 || gy >= 256) return 0;

        // 【魔法修复 2】用无敌的位运算代替痛苦的 floor 和 负数取模
        int cx = gx >> 4;
        int cz = gz >> 4;
        int lx = gx & 15;
        int ly = gy;
        int lz = gz & 15;

        ChunkPos p = { cx, cz };
        auto it = chunks.find(p);
        if (it != chunks.end()) {
            return it->second->GetLight(lx, ly, lz);
        }
        return 15 << 4; // 未加载区块默认充满阳光，哼唧！
    }

    void SetLightAt(int gx, int gy, int gz, uint8_t val) {
        int cx = (int)floor((float)gx / 16.0f);
        int cz = (int)floor((float)gz / 16.0f);
        int lx = gx % 16; if (lx < 0) lx += 16;
        int ly = gy;
        int lz = gz % 16; if (lz < 0) lz += 16;

        if (ly < 0 || ly >= 256) return;
        ChunkPos p = { cx, cz };
        auto it = chunks.find(p);
        if (it != chunks.end()) {
            it->second->SetLight(lx, ly, lz, val);
        }
    }

    void UpdateChunkMesh(ChunkPos pos) {
        if (!chunks.count(pos)) return;
        
        auto getBlockFunc = [this](int gx, int gy, int gz) {
            return this->GetBlockAt(glm::vec3(gx, gy, gz));
        };

        auto getLightFunc = [this](int gx, int gy, int gz) {
            return this->GetLightAt(gx, gy, gz);
        };
        
        chunks[pos]->GenerateMesh(pos.x, pos.z, getBlockFunc, getLightFunc);
    }

    // 梦子看这里！真·无限世界动态更新核心大招！
    // 每一帧都把梦子当前的绝对全局 Position 喂进来，再定一个视野半径（比如 radius=2 代表 5x5 的区块）
    void UpdateInfiniteWorld(glm::vec3 playerPos, int radius) {
        int currentChunkX = (int)playerPos.x >> 4; // 换成顺滑的位移魔法，哼唧~
        int currentChunkZ = (int)playerPos.z >> 4;

        // 魔法优化：记录上一次的区块坐标（可以在类里声明 static 或成员变量）
        static int lastChunkX = -99999;
        static int lastChunkZ = -99999;
        bool chunkChanged = (currentChunkX != lastChunkX || currentChunkZ != lastChunkZ);

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

        if (chunkChanged) {
            lastChunkX = currentChunkX;
            lastChunkZ = currentChunkZ;

            if (terrainQueue.size() > 1) {
                std::sort(terrainQueue.begin(), terrainQueue.end(), [currentChunkX, currentChunkZ](const ChunkPos& a, const ChunkPos& b) {
                    return (std::abs(a.x - currentChunkX) + std::abs(a.z - currentChunkZ)) >
                        (std::abs(b.x - currentChunkX) + std::abs(b.z - currentChunkZ));
                    });
            }
            if (meshQueue.size() > 1) {
                std::sort(meshQueue.begin(), meshQueue.end(), [currentChunkX, currentChunkZ](const ChunkPos& a, const ChunkPos& b) {
                    return (std::abs(a.x - currentChunkX) + std::abs(a.z - currentChunkZ)) >
                        (std::abs(b.x - currentChunkX) + std::abs(b.z - currentChunkZ));
                    });
            }
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
                chunks[posToCreate] = std::make_unique<Chunk>();
                FillFlatChunk(chunks[posToCreate].get(), posToCreate.x, posToCreate.z);

                InitLightForSingleChunk(posToCreate);

                // 【魔法修复 2】如果大树会跨越区块，在它的四周邻居还没齐备前，绝对不能强行生成！
                // 应该检查周围 4 方向区块是否都已就绪：
                bool neighborsReady = chunks.count({ posToCreate.x + 1, posToCreate.z }) &&
                    chunks.count({ posToCreate.x - 1, posToCreate.z }) &&
                    chunks.count({ posToCreate.x, posToCreate.z + 1 }) &&
                    chunks.count({ posToCreate.x, posToCreate.z - 1 });

                if (neighborsReady) {
                    unsigned int chunkSeed = static_cast<unsigned int>(posToCreate.x * 73129 + posToCreate.z * 95121);
                    std::srand(chunkSeed);
                    if ((std::rand() % 100) < 60) {
                        int localX = std::rand() % 12 + 2;
                        int localZ = std::rand() % 12 + 2;
                        int globalX = posToCreate.x * 16 + localX;
                        int globalZ = posToCreate.z * 16 + localZ;

                        // 秒算高度并种树
                        float tx = static_cast<float>(globalX);
                        float tz = static_cast<float>(globalZ);
                        int surfH = static_cast<int>(40.0f + TerrainNoise::GetPerlin(tx * 0.025f, tz * 0.025f) * 25.0f);
                        Structure::GenerateTree(this, globalX, surfH + 1, globalZ);
                    }
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

        if (!meshQueue.empty()) {
        ChunkPos posToUpdate = meshQueue.back();
        meshQueue.pop_back();
        meshSet.erase(posToUpdate);

        if (chunks.count(posToUpdate) > 0) {
            UpdateChunkMesh(posToUpdate);
        }
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

        RecomputeLighting();

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
        // 【魔法修复 1】用无敌的 floor 转成绝对整数坐标
        int gx = (int)std::floor(pos.x);
        int gy = (int)std::floor(pos.y);
        int gz = (int)std::floor(pos.z);

        if (gy < 0 || gy >= 256) return BlockType::AIR;

        // 【核心大修正】用无视正负数的右移（>>）和按位与（&）彻底干碎错位 Bug！
        int cx = gx >> 4;
        int cz = gz >> 4;
        int lx = gx & 15;
        int ly = gy;
        int lz = gz & 15;

        ChunkPos p = { cx, cz };
        auto it = chunks.find(p); // 用 find 代替 count+operator[]，只查一次，更快！
        if (it != chunks.end()) {
            return it->second->blocks[it->second->getIndex(lx, ly, lz)];
        }
        return BlockType::AIR;
    }

    struct LightNode {
        int x, y, z;
    };

    void RecomputeLighting() {
        // 1. 重置所有已加载 Chunk 的光照
        for (auto& pair : chunks) {
            for (int i = 0; i < 16 * 256 * 16; i++) {
                pair.second->light[i] = 0;
            }
        }

        std::queue<LightNode> sunlightQueue;
        std::queue<LightNode> blocklightQueue;

        // 2. 初始化阳光（从每个 Chunk 的 Y=255 向下投射）
        for (auto& pair : chunks) {
            int cx = pair.first.x;
            int cz = pair.first.z;
            for (int x = 0; x < 16; x++) {
                for (int z = 0; z < 16; z++) {
                    int gx = cx * 16 + x;
                    int gz = cz * 16 + z;

                    bool skyVisible = true; // 记录是否还能被天空直射
                    for (int y = 255; y >= 0; y--) {
                        BlockType type = pair.second->blocks[pair.second->getIndex(x, y, z)];
                        if (skyVisible && IsTransparent(type)) {
                            pair.second->light[pair.second->getIndex(x, y, z)] = 15 << 4;
                            sunlightQueue.push({ gx, y, gz });
                        }
                        else {
                            skyVisible = false; // 被挡住了，后续的格子不能直接变15，但需要靠BFS扩散过来
                        }
                    }
                }
            }
        }

        // 3. 初始化光源（寻找所有的 GLOWSTONE）
        for (auto& pair : chunks) {
            int cx = pair.first.x;
            int cz = pair.first.z;

            for (int x = 0; x < 16; x++) {
                for (int y = 0; y < 256; y++) {
                    for (int z = 0; z < 16; z++) {
                        BlockType type = pair.second->blocks[pair.second->getIndex(x, y, z)];
                        if (type == BlockType::GLOWSTONE) {
                            uint8_t currentLight = pair.second->light[pair.second->getIndex(x, y, z)];
                            pair.second->light[pair.second->getIndex(x, y, z)] = (currentLight & 0xF0) | 15;
                            blocklightQueue.push({ cx * 16 + x, y, cz * 16 + z });
                        }
                    }
                }
            }
        }

        // 4. 阳光 BFS 传播
        int dirs[6][3] = {
            { 0, 0, -1 }, { 0, 0, 1 },
            { -1, 0, 0 }, { 1, 0, 0 },
            { 0, -1, 0 }, { 0, 1, 0 }
        };

        while (!sunlightQueue.empty()) {
            LightNode node = sunlightQueue.front();
            sunlightQueue.pop();

            uint8_t currentLightVal = GetLightAt(node.x, node.y, node.z);
            uint8_t currentSun = (currentLightVal >> 4) & 0xF;
            if (currentSun == 0) continue;

            for (int d = 0; d < 6; d++) {
                int nx = node.x + dirs[d][0];
                int ny = node.y + dirs[d][1];
                int nz = node.z + dirs[d][2];

                if (ny < 0 || ny >= 256) continue;

                // 使用安全的整数位移来获取区块坐标
                int ncx = nx >> 4;
                int ncz = nz >> 4;
                if (chunks.count({ ncx, ncz }) == 0) continue;

                BlockType neighborType = GetBlockAt(glm::vec3(nx, ny, nz));
                if (!IsTransparent(neighborType)) continue;

                uint8_t neighborLightVal = GetLightAt(nx, ny, nz);
                uint8_t neighborSun = (neighborLightVal >> 4) & 0xF;

                uint8_t nextSun = 0;
                // 只有当当前格子是直射光(15)，且方向是向下(d==4)时，下一格才保持15
                if (d == 4 && currentSun == 15) {
                    nextSun = 15;
                }
                else {
                    nextSun = currentSun - 1;
                }

                // 严格大于才更新，避免15跟15之间无限死循环
                if (nextSun > 0 && neighborSun < nextSun) {
                    SetLightAt(nx, ny, nz, (neighborLightVal & 0x0F) | (nextSun << 4));
                    sunlightQueue.push({ nx, ny, nz });
                }
            }
        }

        // 5. 光源光 BFS 传播
        while (!blocklightQueue.empty()) {
            LightNode node = blocklightQueue.front();
            blocklightQueue.pop();

            uint8_t currentLightVal = GetLightAt(node.x, node.y, node.z);
            uint8_t currentBlock = currentLightVal & 0xF;
            if (currentBlock == 0) continue;

            for (int d = 0; d < 6; d++) {
                int nx = node.x + dirs[d][0];
                int ny = node.y + dirs[d][1];
                int nz = node.z + dirs[d][2];

                if (ny < 0 || ny >= 256) continue;

                int ncx = (int)floor((float)nx / 16.0f);
                int ncz = (int)floor((float)nz / 16.0f);
                ChunkPos nPos = { ncx, ncz };
                if (chunks.count(nPos) == 0) continue;

                BlockType neighborType = GetBlockAt(glm::vec3(nx, ny, nz));
                if (!IsTransparent(neighborType)) continue;

                uint8_t neighborLightVal = GetLightAt(nx, ny, nz);
                uint8_t neighborBlock = neighborLightVal & 0xF;

                uint8_t nextBlock = currentBlock - 1;
                if (neighborBlock < nextBlock) {
                    SetLightAt(nx, ny, nz, (neighborLightVal & 0xF0) | nextBlock);
                    blocklightQueue.push({ nx, ny, nz });
                }
            }
        }
    }
    void PropagateLocalLightingChange(int gx, int gy, int gz) {
        std::queue<LightNode> blockAddQueue;
        std::queue<LightRemovalNode> blockReleaseQueue;
        std::queue<LightNode> sunAddQueue;
        std::queue<LightRemovalNode> sunReleaseQueue;

        int dirs[6][3] = { {0,0,-1}, {0,0,1}, {-1,0,0}, {1,0,0}, {0,-1,0}, {0,1,0} };
        BlockType currentType = GetBlockAt(glm::vec3(gx, gy, gz));

        // 【核心大重构：幽灵残光精准捕获魔法】
        uint8_t oldLightVal = GetLightAt(gx, gy, gz);
        uint8_t oldBlockLight = oldLightVal & 0x0F;
        uint8_t oldSunLight = (oldLightVal >> 4) & 0x0F;

        // 梦子看这里！如果这一格现在是坚硬固体，说明它是刚放下的！
        // 它在没放方块前的上一帧，百分之百继承了周围空气的圣光！我们去偷看它周围 6 个邻居的最高阳光值！
        if (!IsTransparent(currentType)) {
            uint8_t maxNeighborSun = 0;
            for (int d = 0; d < 6; d++) {
                int nx = gx + dirs[d][0], ny = gy + dirs[d][1], nz = gz + dirs[d][2];
                if (ny >= 0 && ny < 256) {
                    uint8_t nSun = (GetLightAt(nx, ny, nz) >> 4) & 0x0F;
                    if (nSun > maxNeighborSun) maxNeighborSun = nSun;
                }
            }
            // 如果周围有光，或者我头顶就是苍穹，说明我这一格原本绝对充满了直射阳光（15点）！
            if (oldSunLight == 0 && maxNeighborSun > 0) {
                bool isDirectSky = true;
                for (int ty = 255; ty > gy; ty--) {
                    if (!IsTransparent(GetBlockAt(glm::vec3(gx, ty, gz)))) { isDirectSky = false; break; }
                }
                oldSunLight = isDirectSky ? 15 : (maxNeighborSun - 1);
                if (oldSunLight < 0) oldSunLight = 0;
            }
        }

        // ====================================================
        // 【第一阶段：黑暗瘟疫连锁爆燃】
        // ====================================================

        // 情况 A：如果在黑暗中放下了璀璨的萤石
        if (currentType == BlockType::GLOWSTONE) {
            SetLightAt(gx, gy, gz, (oldLightVal & 0xF0) | 15);
            blockAddQueue.push({ gx, gy, gz });
        }
        // 情况 B：放下了阻挡一切光芒的普通固体方块（斩断一整列阳光柱！）
        else if (oldBlockLight > 0 || oldSunLight > 0 || !IsTransparent(currentType)) {

            // 黑暗种子成功觉醒，带队出发！
            if (oldBlockLight > 0) blockReleaseQueue.push({ gx, gy, gz, oldBlockLight });
            if (oldSunLight > 0) sunReleaseQueue.push({ gx, gy, gz, oldSunLight });

            // 这一格被塞进了石头，魔力账本瞬间强行抹清为 0
            SetLightAt(gx, gy, gz, 0);

            // 1. 方块光黑暗扩散
            while (!blockReleaseQueue.empty()) {
                auto node = blockReleaseQueue.front(); blockReleaseQueue.pop();
                for (int d = 0; d < 6; d++) {
                    int nx = node.x + dirs[d][0], ny = node.y + dirs[d][1], nz = node.z + dirs[d][2];
                    if (ny < 0 || ny >= 256) continue;
                    uint8_t nVal = GetLightAt(nx, ny, nz);
                    uint8_t nBlock = nVal & 0x0F;
                    if (nBlock != 0 && nBlock == node.val - 1) {
                        blockReleaseQueue.push({ nx, ny, nz, nBlock });
                        SetLightAt(nx, ny, nz, nVal & 0xF0);
                    }
                    else if (nBlock >= node.val) blockAddQueue.push({ nx, ny, nz });
                }
            }

            // 2. 阳光黑暗扩散（轰！！！顺着悬空方块正下方，一路把阳光柱无情吞噬！）
            while (!sunReleaseQueue.empty()) {
                auto node = sunReleaseQueue.front(); sunReleaseQueue.pop();
                for (int d = 0; d < 6; d++) {
                    int nx = node.x + dirs[d][0], ny = node.y + dirs[d][1], nz = node.z + dirs[d][2];
                    if (ny < 0 || ny >= 256) continue;

                    uint8_t nVal = GetLightAt(nx, ny, nz);
                    uint8_t nSun = (nVal >> 4) & 0x0F;

                    // 阳光向下传播（d == 4）且当时是直射（15点）时，下一格也是15点！
                    uint8_t expectedSun = (d == 4 && node.val == 15) ? 15 : (node.val - 1);
                    if (node.val == 15 && d == 4) expectedSun = 15;

                    if (nSun != 0 && nSun == expectedSun) {
                        sunReleaseQueue.push({ nx, ny, nz, nSun });
                        SetLightAt(nx, ny, nz, nVal & 0x0F); // 抹除邻居的阳光
                        SafePushMeshQueue({ nx >> 4, nz >> 4 });
                    }
                    else if (nSun >= expectedSun) {
                        sunAddQueue.push({ nx, ny, nz });
                    }
                }
            }
        }

        // 情况 C：单纯挖开了一个原生的非光源方块（空气倒灌）
        if (IsTransparent(currentType) && currentType != BlockType::GLOWSTONE) {
            bool isSkyVisible = true;
            for (int ty = 255; ty > gy; ty--) {
                if (!IsTransparent(GetBlockAt(glm::vec3(gx, ty, gz)))) { isSkyVisible = false; break; }
            }
            if (isSkyVisible) {
                uint8_t curVal = GetLightAt(gx, gy, gz);
                SetLightAt(gx, gy, gz, (curVal & 0x0F) | (15 << 4));
                sunAddQueue.push({ gx, gy, gz });
                for (int dy = gy - 1; dy >= 0; dy--) {
                    if (!IsTransparent(GetBlockAt(glm::vec3(gx, dy, gz)))) break;
                    uint8_t downVal = GetLightAt(gx, dy, gz);
                    SetLightAt(gx, dy, gz, (downVal & 0x0F) | (15 << 4));
                    sunAddQueue.push({ gx, dy, gz });
                }
            }
            for (int d = 0; d < 6; d++) {
                int nx = gx + dirs[d][0], ny = gy + dirs[d][1], nz = gz + dirs[d][2];
                if (ny < 0 || ny >= 256) continue;
                uint8_t nVal = GetLightAt(nx, ny, nz);
                if ((nVal & 0x0F) > 0) blockAddQueue.push({ nx, ny, nz });
                if (((nVal >> 4) & 0x0F) > 0) sunAddQueue.push({ nx, ny, nz });
            }
        }

        // ====================================================
        // 【第二阶段：圣光复燃法阵 —— 重新照亮未受波及的阴影】
        // ====================================================
        while (!blockAddQueue.empty()) {
            auto node = blockAddQueue.front(); blockAddQueue.pop();
            uint8_t currentBlock = GetLightAt(node.x, node.y, node.z) & 0x0F;
            for (int d = 0; d < 6; d++) {
                int nx = node.x + dirs[d][0], ny = node.y + dirs[d][1], nz = node.z + dirs[d][2];
                if (ny < 0 || ny >= 256) continue;
                if (!IsTransparent(GetBlockAt(glm::vec3(nx, ny, nz)))) continue;
                uint8_t nVal = GetLightAt(nx, ny, nz);
                uint8_t nBlock = nVal & 0x0F;
                if (currentBlock > 1 && nBlock < currentBlock - 1) {
                    SetLightAt(nx, ny, nz, (nVal & 0xF0) | (currentBlock - 1));
                    blockAddQueue.push({ nx, ny, nz });
                    SafePushMeshQueue({ nx >> 4, nz >> 4 });
                }
            }
        }

        while (!sunAddQueue.empty()) {
            auto node = sunAddQueue.front(); sunAddQueue.pop();
            uint8_t currentSun = (GetLightAt(node.x, node.y, node.z) >> 4) & 0x0F;
            for (int d = 0; d < 6; d++) {
                int nx = node.x + dirs[d][0], ny = node.y + dirs[d][1], nz = node.z + dirs[d][2];
                if (ny < 0 || ny >= 256) continue;
                if (!IsTransparent(GetBlockAt(glm::vec3(nx, ny, nz)))) continue;
                uint8_t nVal = GetLightAt(nx, ny, nz);
                uint8_t nSun = (nVal >> 4) & 0x0F;
                uint8_t nextSun = (d == 4 && currentSun == 15) ? 15 : (currentSun - 1);
                if (currentSun > 0 && nSun < nextSun) {
                    SetLightAt(nx, ny, nz, (nVal & 0x0F) | (nextSun << 4));
                    sunAddQueue.push({ nx, ny, nz });
                    SafePushMeshQueue({ nx >> 4, nz >> 4 });
                }
            }
        }
    }

    void InitLightForSingleChunk(const ChunkPos& pos) {
        auto it = chunks.find(pos);
        if (it == chunks.end()) return;

        std::queue<LightNode> localSunQueue;

        // 从新区块的 Y=255 向下进行阳光直射皮肤泼洒
        for (int x = 0; x < 16; x++) {
            for (int z = 0; z < 16; z++) {
                int gx = pos.x * 16 + x;
                int gz = pos.z * 16 + z;
                for (int y = 255; y >= 0; y--) {
                    BlockType type = it->second->blocks[it->second->getIndex(x, y, z)];
                    if (IsTransparent(type)) {
                        // 直射阳光充满高4位
                        it->second->light[it->second->getIndex(x, y, z)] = 15 << 4;
                        localSunQueue.push({ gx, y, gz });
                    }
                    else {
                        break; // 被不透明方块（地表草地）挡住了，中止直射
                    }
                }
            }
        }
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