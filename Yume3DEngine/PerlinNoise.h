#pragma once
#include <glm/glm.hpp>
#include <cmath>

class TerrainNoise {
private:
    // 现代 C++ 规范：将神圣的哈希魔数全部归位
    static constexpr int PRIME_X = 374761393;  // 正确的 PRIME32_3，感谢梦子的火眼金睛！
    static constexpr int PRIME_Z = -1029149011; // 经典的位移混淆质数
    static constexpr int PRIME_SEED = 668265261; // 专门用来引入种子的神圣质数
    static constexpr int HASH_SCRAMBLE = 1274126177;

    // 1. 完美融入世界种子的伪随机渐变查找表
    static glm::vec2 GetGradient(int x, int z) {
        int seed = 124325213431;
        // 先用种子乘以特定质数打碎，再与 X、Z 轴的哈希结果强力揉碎在一起
        int n = x * PRIME_X + z * PRIME_Z + seed * PRIME_SEED;

        // 经典的雪崩大洗牌机制
        n = (n ^ (n >> 13)) * HASH_SCRAMBLE;

        // 映射到 0 ~ 2*PI 弧度
        float angle = static_cast<float>(n & 2147483647) * 0.0000000029122f;
        return glm::vec2(cos(angle), sin(angle));
    }

    static float Fade(float t) { return t * t * (3.0f - 2.0f * t); }
    static float Lerp(float a, float b, float t) { return a + t * (b - a); }


public:
    // 2. 核心大招：对外暴露的带种子二维柏林噪声
    static float GetPerlin(float x, float z) {
        int x0 = static_cast<int>(floor(x)); int x1 = x0 + 1;
        int z0 = static_cast<int>(floor(z)); int z1 = z0 + 1;

        float tx = x - x0; float tz = z - z0;
        float fx = Fade(tx); float fz = Fade(tz);

        //  在这里把 seed 魔法完美派发给四个顶点！
        float d00 = glm::dot(GetGradient(x0, z0), glm::vec2(tx, tz));
        float d10 = glm::dot(GetGradient(x1, z0), glm::vec2(tx - 1.0f, tz));
        float d01 = glm::dot(GetGradient(x0, z1), glm::vec2(tx, tz - 1.0f));
        float d11 = glm::dot(GetGradient(x1, z1), glm::vec2(tx - 1.0f, tz - 1.0f));

        float ix0 = Lerp(d00, d10, fx);
        float ix1 = Lerp(d01, d11, fx);
        return Lerp(ix0, ix1, fz);
    }
};
