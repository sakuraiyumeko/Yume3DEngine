#pragma once
#include <glm/glm.hpp>
#include <cmath>

class TerrainNoise {
private:
    // 1. 梦子手撕的高贵伪随机渐变向量查找表（经典的 8 个方向）
    static glm::vec2 GetGradient(int x, int z) {
        // 利用极强的高频数学哈希公式，把坐标揉碎，变出绝对公允的伪随机方向
        int n = x * 374761393 + z * 668265263;
        n = (n ^ (n >> 13)) * 1274126177;
        float angle = static_cast<float>(n & 2147483647) * 0.0000000029122f; // 映射到 0 ~ 2*PI
        return glm::vec2(cos(angle), sin(angle));
    }

    // 2. 高阶三阶 Hermite 缓动插值曲线（让山峦平滑起伏的核心魔法，f(x) = 3x^2 - 2x^3）
    static float Fade(float t) { return t * t * (3.0f - 2.0f * t); }

    static float Lerp(float a, float b, float t) { return a + t * (b - a); }

public:
    // 3.  核心大招：基础二维柏林噪声（吐出 -1.0 到 1.0 的平滑波动值）
    static float GetPerlin(float x, float z) {
        int x0 = static_cast<int>(floor(x)); int x1 = x0 + 1;
        int z0 = static_cast<int>(floor(z)); int z1 = z0 + 1;

        float tx = x - x0; float tz = z - z0;
        float fx = Fade(tx); float fz = Fade(tz);

        // 算出四个网格顶点到当前采样点的距离向量，并和对应的伪随机渐变向量做点积
        float d00 = glm::dot(GetGradient(x0, z0), glm::vec2(tx, tz));
        float d10 = glm::dot(GetGradient(x1, z0), glm::vec2(tx - 1.0f, tz));
        float d01 = glm::dot(GetGradient(x0, z1), glm::vec2(tx, tz - 1.0f));
        float d11 = glm::dot(GetGradient(x1, z1), glm::vec2(tx - 1.0f, tz - 1.0f));

        // 两次 X 轴插值，一次 Z 轴插值，纯正的双线性数学凝结！
        float ix0 = Lerp(d00, d10, fx);
        float ix1 = Lerp(d01, d11, fx);
        return Lerp(ix0, ix1, fz);
    }
};
