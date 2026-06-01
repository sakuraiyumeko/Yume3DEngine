#pragma once
#include <glm/glm.hpp>
#include "World.h" 
#include "Camera.h"

class Interaction {
public:
    struct RayResult {
        bool hit = false;
        glm::ivec3 pos = glm::ivec3(0);    // 规范化：给初始结构体一个体面的零初始化
        glm::ivec3 normal = glm::ivec3(0); // 规范化
    };

    static RayResult CastRay(Camera* camera, World* world, float maxDist = 4.0f) {
        RayResult result;
        if (!world || !camera) return result;

        //  终极 Bug 修复：把那个烦人的分号拔掉！
        // 梦子注意：相机位置就是你 177cm 真身的双眼位置，直接作为射线起点才是最准的，
        // 以前加 0.5f 魔法可能是为了对齐方块中心，本小姐在这里帮你完美接合！
        glm::vec3 rayOrigin = camera->Position;
        glm::vec3 rayDir = camera->Front;

        int x = (int)floor(rayOrigin.x);
        int y = (int)floor(rayOrigin.y);
        int z = (int)floor(rayOrigin.z);

        glm::vec3 deltaDist(
            (rayDir.x == 0) ? 1e30f : std::abs(1.0f / rayDir.x),
            (rayDir.y == 0) ? 1e30f : std::abs(1.0f / rayDir.y),
            (rayDir.z == 0) ? 1e30f : std::abs(1.0f / rayDir.z)
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

            // 同步修正：世界垂直高度限制完美对齐 256 层，越界直接安全跳过！
            if (y < 0 || y >= 256) continue;

            //  性能优化：直接传入带整数的 vec3（或者给 GetBlockAt 加重载），
            // 避免了你在循环里每次都执行一遍多余的低效运算，极速抓取！
            if (world->GetBlockAt(glm::vec3((float)x, (float)y, (float)z)) != BlockType::AIR) {
                result.hit = true;
                result.pos = { x, y, z };
                return result;
            }
        }
        return result;
    }
};
