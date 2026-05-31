#pragma once
#include <glm/glm.hpp>
#include "Camera.h"


class Physics {
public:
    // 1. 核心判定：某个坐标点是否是实心方块
    static bool IsSolid(const glm::vec3& pos, World* world) {
        if (!world) return false;

        // 虚空挡板：如果掉得太深就当作踩在实体上
        //if (pos.y <= -0.5f) return true;

        // 直接用你在 World 里面辛辛苦苦写好的全局坐标查询函数！
        BlockType block = world->GetBlockAt(pos);

        // 只要查询出来的不是空气，那就是踩到实体啦！
        return block != BlockType::AIR;
    }


    // 2. 盒式碰撞检测（梦子 177cm 的全身结界）
    static bool CheckCollision(const glm::vec3& pos, Camera* camera, World* world) {
        float r = camera->PlayerRadius;
        // 梦子听好了！不要用固定的 PlayerHeight
        // 我们根据当前的眼睛高度，加上头顶到眼睛的那一小段距离 (比如 0.1f ~ 0.2f)
        float currentHeight = camera->EyeHeight + 0.3f;
        float eye = camera->EyeHeight;

        float footY = pos.y - eye + 0.05f;           // 脚底
        float bodyY = pos.y;                         // 躯干（直接用眼睛高度附近）
        float headY = pos.y + (currentHeight - eye) - 0.05f; // 真正的头顶

        float heights[3] = { footY, bodyY, headY };
        for (float hy : heights) {
            if (IsSolid({ pos.x - r, hy, pos.z - r }, world)) return true;
            if (IsSolid({ pos.x + r, hy, pos.z - r }, world)) return true;
            if (IsSolid({ pos.x - r, hy, pos.z + r }, world)) return true;
            if (IsSolid({ pos.x + r, hy, pos.z + r }, world)) return true;
        }
        return false;
    }
};