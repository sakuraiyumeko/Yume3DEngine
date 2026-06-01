#pragma once

#ifndef INPUT_H
#define INPUT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Input {
private:
    static inline float LastX = 400.0f;
    static inline float LastY = 300.0f;
    static inline bool FirstMouse = true;
    static inline float DeltaTime = 0.0f;
    static inline float LastFrame = 0.0f;

    // 梦子注意：保留你原汁原味的按键记忆状态
    static inline bool KeysChanged[1024] = { false };

public:
    static void Update() {
        float currentFrame = static_cast<float>(glfwGetTime());
        DeltaTime = currentFrame - LastFrame;
        LastFrame = currentFrame;
    }

    static float GetDeltaTime() { return DeltaTime; }

    // 完美复活！原汁原味的连续按键判定接口，绝不报错
    static bool IsKeyPressed(GLFWwindow* window, int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    // 完美复活！原汁原味的单次单帧按键锁定接口
    static bool IsKeyJustPressed(GLFWwindow* window, int key) {
        bool isPressed = (glfwGetKey(window, key) == GLFW_PRESS);
        bool justPressed = isPressed && !KeysChanged[key];
        KeysChanged[key] = isPressed;
        return justPressed;
    }

    static void MouseCallback(GLFWwindow* window, double xposIn, double yposIn) {}

    // 完美复活！梦子专属的原生无边界鼠标绝对偏移量读取函数
    static glm::vec2 GetMouseOffset(GLFWwindow* handle) {
        double xpos, ypos;
        glfwGetCursorPos(handle, &xpos, &ypos);

        static bool firstMouse = true;
        static double lastX = xpos;
        static double lastY = ypos;

        if (firstMouse) {
            lastX = xpos; lastY = ypos;
            firstMouse = false;
        }

        float xoffset = static_cast<float>(xpos - lastX);
        float yoffset = static_cast<float>(lastY - ypos);

        lastX = xpos;
        lastY = ypos;

        return glm::vec2(xoffset, yoffset);
    }
};

#endif
