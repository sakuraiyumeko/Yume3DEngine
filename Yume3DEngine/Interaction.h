#pragma once
#include <glm/glm.hpp>
// 别忘了包含你的世界头文件哦！
#include "World.h" 
#include "Camera.h"

class Interaction {
public:
    struct RayResult {
        bool hit = false;
        glm::ivec3 pos;
        glm::ivec3 normal;
    };

    static RayResult CastRay(Camera* camera, World* world, float maxDist = 3.0f) {//触及距离
        RayResult result;
        if (!world || !camera) return result; // 安全第一的防爆魔法！

        glm::vec3 rayOrigin = camera->Position; + glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 rayDir = camera->Front;

        int x = (int)floor(rayOrigin.x);
        int y = (int)floor(rayOrigin.y);
        int z = (int)floor(rayOrigin.z);

        glm::vec3 deltaDist(
            (rayDir.x == 0) ? 1e30f : abs(1.0f / rayDir.x),
            (rayDir.y == 0) ? 1e30f : abs(1.0f / rayDir.y),
            (rayDir.z == 0) ? 1e30f : abs(1.0f / rayDir.z)
        );

        int stepX = (rayDir.x < 0) ? -1 : 1;
        int stepY = (rayDir.y < 0) ? -1 : 1;
        int stepZ = (rayDir.z < 0) ? -1 : 1;

        glm::vec3 sideDist;
        sideDist.x = (rayDir.x < 0) ? (rayOrigin.x - x) * deltaDist.x : (x + 1.0f - rayOrigin.x) * deltaDist.x;
        sideDist.y = (rayDir.y < 0) ? (rayOrigin.y - y) * deltaDist.y : (y + 1.0f - rayOrigin.y) * deltaDist.y;
        sideDist.z = (rayDir.z < 0) ? (rayOrigin.z - z) * deltaDist.z : (z + 1.0f - rayOrigin.z) * deltaDist.z;

        float distance = 0;
        while (distance < maxDist) {
            if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
                distance = sideDist.x; sideDist.x += deltaDist.x; x += stepX;
                result.normal = { -stepX, 0, 0 };
            }
            else if (sideDist.y < sideDist.z) {
                distance = sideDist.y; sideDist.y += deltaDist.y; y += stepY;
                result.normal = { 0, -stepY, 0 };
            }
            else {
                distance = sideDist.z; sideDist.z += deltaDist.z; z += stepZ;
                result.normal = { 0, 0, -stepZ };
            }

            // 扔掉以前的 0~16 限制！让射线在多区块间穿梭！
            // 高度限制可以保留，因为你的世界垂直高度目前是 0~16 
            if (y < 0 || y >= 256) continue;

            // 直接用当前射线所在的全局整数坐标转成 vec3 去查询！
            if (world->GetBlockAt(glm::vec3(x, y, z)) != BlockType::AIR) {
                result.hit = true;
                result.pos = { x, y, z };
                return result; // 抓到方块了，收工！
            }
        }
        return result;
    }
};
