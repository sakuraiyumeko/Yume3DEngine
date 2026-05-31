#pragma once

#ifndef INPUT_H
#define INPUT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Input {
private:
    // 1. 鼠标时空隐私数据（完全内聚在私有结界里，外面再也别想偷看！）
    static inline float LastX = 400.0f;
    static inline float LastY = 300.0f;
    static inline float XOffset = 0.0f;
    static inline float YOffset = 0.0f;
    static inline bool FirstMouse = true;

    static inline float DeltaTime = 0.0f;
    static inline float LastFrame = 0.0f;

    // 下面这两个大数组的声明也要同步修正顺序噢，哼唧！
    static inline bool Keys[1024] = { false };
    static inline bool KeysChanged[1024] = { false };

public:
    // 【核心大招一：时钟与状态现场熔炼术】—— 每一帧都要在循环最开头调用它！
    static void Update() {
        // 计算时钟差，确保梦子 9km/h 快走在任何配置的显卡下都绝对匀速
        float currentFrame = static_cast<float>(glfwGetTime());
        DeltaTime = currentFrame - LastFrame;
        LastFrame = currentFrame;

    }

    // 【核心大招二：高雅的全局时钟外借接口】
    static float GetDeltaTime() { return DeltaTime; }

    // 【核心大招三：高阶按键判定接口】
    // 只要按键被按住，就一直返回 true（用来驱动 WASD 疯狂走位和空格飞天！）
    static bool IsKeyPressed(GLFWwindow* window, int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    // 【卷死两厂的高阶独家符文】：只有在“刚按下去的那一帧”返回 true，长按无效！
    // 梦子快看！有了这个接口，你未来所有的全屏切换、或者打开背包，连 static 都不用写了，直接一行搞定！
    static bool IsKeyJustPressed(GLFWwindow* window, int key) {
        bool isPressed = (glfwGetKey(window, key) == GLFW_PRESS);
        bool justPressed = isPressed && !KeysChanged[key];
        KeysChanged[key] = isPressed; // 同步按键记忆锁
        return justPressed;
    }

    // 【核心大招四：原生鼠标位移接收回调阵法】
    // 梦子注意！这个必须是符合 GLFW 规矩的静态回调函数噢！
    static void MouseCallback(GLFWwindow* window, double xposIn, double yposIn) {

        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (FirstMouse) {
            LastX = xpos;
            LastY = ypos;
            FirstMouse = false;
        }

        // 计算硬件级偏移
        XOffset = xpos - LastX;
        YOffset = LastY - ypos; // 反转 Y 轴，抵抗世界倒退！

        LastX = xpos;
        LastY = ypos;
    }

    // 【核心大招五：对外派发鼠标运动偏移量的接口】
    static glm::vec2 GetMouseOffset() {
        glm::vec2 result(XOffset*2, YOffset*2);

        // 刷新鼠标每帧的相对偏移量，等被拿走后就自动清零，超级干净！
        XOffset = 0.0f;
        YOffset = 0.0f;

        return result;
    }
};

#endif
