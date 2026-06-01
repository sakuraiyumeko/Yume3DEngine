// 核心大招：在这里同时包含两个头文件，因为在 .cpp 内部它们已经【彻底解耦】了，绝对不会死循环！
#include "Structure.h"
#include "World.h" 
#include <cmath>
#include <cstdlib>

void Structure::SetGlobalBlockSafe(World* world, int gx, int gy, int gz, BlockType type) {
    if (!world || gy < 0 || gy >= 256) return;

    int cx = static_cast<int>(std::floor(static_cast<float>(gx) / 16.0f));
    int cz = static_cast<int>(std::floor(static_cast<float>(gz) / 16.0f));

    int lx = gx % 16; if (lx < 0) lx += 16;
    int lz = gz % 16; if (lz < 0) lz += 16;

    ChunkPos cp = { cx, cz };
    if (world->chunks.count(cp) > 0) {
        world->chunks[cp]->SetBlock(lx, gy, lz, type);
    }
}

void Structure::GenerateTree(World* world, int rootX, int rootY, int rootZ) {
    if (!world) return;

    // 1. 稳稳地拔地而起 4 格高的神圣树干 (4 代表 LOG)
    for (int h = 0; h < 4; h++) {
        SetGlobalBlockSafe(world, rootX, rootY + h, rootZ, static_cast<BlockType>(4));
    }

    // 2. 树干第 3 层周围（高度 h = 2）：画一个 5x5 的树叶厚片，掐掉四个角让它圆润 (5 代表 LEAVES)
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            if (std::abs(dx) == 2 && std::abs(dz) == 2) continue;
            if (dx == 0 && dz == 0) continue;
            SetGlobalBlockSafe(world, rootX + dx, rootY + 2, rootZ + dz, static_cast<BlockType>(5));
        }
    }

    // 3. 树干第 4 层周围（高度 h = 3）：画一个 3x3 的树叶薄片
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (dx == 0 && dz == 0) continue;
            SetGlobalBlockSafe(world, rootX + dx, rootY + 3, rootZ + dz, static_cast<BlockType>(5));
        }
    }

    // 4. 在树干最顶端（高度 h = 4）盖上最后的十字小树顶 
    SetGlobalBlockSafe(world, rootX, rootY + 4, rootZ, static_cast<BlockType>(5));
    SetGlobalBlockSafe(world, rootX + 1, rootY + 4, rootZ, static_cast<BlockType>(5));
    SetGlobalBlockSafe(world, rootX - 1, rootY + 4, rootZ, static_cast<BlockType>(5));
    SetGlobalBlockSafe(world, rootX, rootY + 4, rootZ + 1, static_cast<BlockType>(5));
    SetGlobalBlockSafe(world, rootX, rootY + 4, rootZ - 1, static_cast<BlockType>(5));
}
