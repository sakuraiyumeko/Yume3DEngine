#pragma once

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <glm/glm.hpp>
#include "Chunk.h" // 确保 BlockType 枚举可见

// 高贵的前置声明，彻底切断头文件套娃死循环！
class World;

class Structure {
private:
    // 只有声明，没有实现！
    static void SetGlobalBlockSafe(World* world, int gx, int gy, int gz, BlockType type);

public:
    // 只有声明，没有实现！
    static void GenerateTree(World* world, int rootX, int rootY, int rootZ);

    static void GenerateMysteryRoom(World* world, int startX, int startY, int startZ) {}
};

#endif
