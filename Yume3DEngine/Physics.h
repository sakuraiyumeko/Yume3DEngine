#pragma once
#include <glm/glm.hpp>
#include "Camera.h"
#include "World.h" // 确保世界魔导书完美可见！

class Physics {
public:
    // 核心判定：某个坐标点是否是实心方块
    static bool IsSolid(const glm::vec3& pos, World* world) {
        if (!world) return false;
        BlockType block = world->GetBlockAt(pos);
        return block != BlockType::AIR;
    }

    // 盒式碰撞检测（梦子 177cm 的全身结界）
    static bool CheckCollision(const glm::vec3& pos, Camera* camera, World* world) {
        //  安全第一防御：如果世界还不存在，直接判定没有碰撞，防爆安全！
        if (!world || !camera) return false;

        float r = camera->PlayerRadius;
        float currentHeight = camera->EyeHeight + 0.18f; // 177cm 的高贵总身高
        float eye = camera->EyeHeight;

        // 极其严谨的空间低维五层切片高度算子
        float footY = pos.y - eye + 0.05f;                 // 1层：脚底
        float kneeY = pos.y - eye + (currentHeight * 0.25f); // 2层：膝盖
        float waistY = pos.y - eye + (currentHeight * 0.50f); // 3层：腰部
        float chestY = pos.y - eye + (currentHeight * 0.75f); // 4层：胸膛
        float headY = pos.y - eye + currentHeight - 0.05f;   // 5层：头顶

        float heights[5] = { footY, kneeY, waistY, chestY, headY };

        for (float hy : heights) {
            // 四角盒子（AABB）环形探测扫描
            //  魔法加速小技巧：只要任何一个角踩到了实心方块，立刻 short-circuit（提前返回 true），绝对不浪费多余的 CPU 计算！
            if (IsSolid({ pos.x - r, hy, pos.z - r }, world)) return true;
            if (IsSolid({ pos.x + r, hy, pos.z - r }, world)) return true;
            if (IsSolid({ pos.x - r, hy, pos.z + r }, world)) return true;
            if (IsSolid({ pos.x + r, hy, pos.z + r }, world)) return true;
        }
        return false;
    }
};
