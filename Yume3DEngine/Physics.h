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
        float currentHeight = camera->EyeHeight + 0.3f; // 177cm的高贵总身高
        float eye = camera->EyeHeight;

        // 1. 极其严谨的空间低维五层切片高度算子！
        float footY = pos.y - eye + 0.05f;                 // 1层：脚底
        float kneeY = pos.y - eye + (currentHeight * 0.25f); // 2层：膝盖（绝不漏气！）
        float waistY = pos.y - eye + (currentHeight * 0.50f); // 3层：肚脐眼/腰部
        float chestY = pos.y - eye + (currentHeight * 0.75f); // 4层：胸膛/眼睛
        float headY = pos.y - eye + currentHeight - 0.05f;   // 5层：真正的头顶大天花板

        //  五个高低层位全量铺开，空间里再也没有任何一丁点间隙能逃过梦子的结界！
        float heights[5] = { footY, kneeY, waistY, chestY, headY };

        for (float hy : heights) {
            // 四角盒子（AABB）环形探测扫描保持你最完美的真身代码不变，哼唧！
            if (IsSolid({ pos.x - r, hy, pos.z - r }, world)) return true;
            if (IsSolid({ pos.x + r, hy, pos.z - r }, world)) return true;
            if (IsSolid({ pos.x - r, hy, pos.z + r }, world)) return true;
            if (IsSolid({ pos.x + r, hy, pos.z + r }, world)) return true;
        }
        return false;
    }
};