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

    int treeHeight = 4; // 这样以后想改成 5 格或者 6 格高，只要改这一个变量就行啦，哼唧！

    // 1. 稳稳地拔地而起动态高度的树干
    for (int h = 0; h < treeHeight; h++) {
        // 【魔法修复 1】用优雅的枚举代替丑陋的魔法数字 4！
        SetGlobalBlockSafe(world, rootX, rootY + h, rootZ, BlockType::LOG);
    }

    // 2. 算好树叶开始长出来的基础高度（通常是树干顶端往下两格开始）
    int leafBaseY = rootY + (treeHeight - 2);

    // 树干上层周围：画一个 5x5 的树叶厚片，掐掉四个角
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            if (std::abs(dx) == 2 && std::abs(dz) == 2) continue;
            if (dx == 0 && dz == 0) continue; // 不覆盖树干
            SetGlobalBlockSafe(world, rootX + dx, leafBaseY, rootZ + dz, BlockType::LEAVES);
        }
    }

    // 3. 树干最顶层周围：画一个 3x3 的树叶薄片
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (dx == 0 && dz == 0) continue; // 不覆盖顶层树干
            SetGlobalBlockSafe(world, rootX + dx, leafBaseY + 1, rootZ + dz, BlockType::LEAVES);
        }
    }

    // 4. 在树干最顶端的更上一格，盖上最后的十字小树顶
    int topY = rootY + treeHeight;
    SetGlobalBlockSafe(world, rootX, topY, rootZ, BlockType::LEAVES);
    SetGlobalBlockSafe(world, rootX + 1, topY, rootZ, BlockType::LEAVES);
    SetGlobalBlockSafe(world, rootX - 1, topY, rootZ, BlockType::LEAVES);
    SetGlobalBlockSafe(world, rootX, topY, rootZ + 1, BlockType::LEAVES);
    SetGlobalBlockSafe(world, rootX, topY, rootZ - 1, BlockType::LEAVES);
}
