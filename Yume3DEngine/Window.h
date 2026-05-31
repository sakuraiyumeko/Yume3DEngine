#pragma once

#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Window {
public:
    GLFWwindow* Handle; // 真正的操作系统窗口魔法句柄

    // 窗口时空记忆碎片（被完美收纳进类内部，外面再也看不见全局垃圾了，哼！）
    bool IsFullscreen;
    int WindowedX, WindowedY;
    int WindowedWidth, WindowedHeight;

    // 构造函数：诞生即把所有显卡初始化的累赘活计全部在后台搞定！
    Window(int width, int height, const char* title)
        : Handle(nullptr), IsFullscreen(false),
        WindowedX(100), WindowedY(100), WindowedWidth(width), WindowedHeight(height) {

        // 1. GLFW 基础仪式开启
        if (!glfwInit()) {
            std::cout << "Failed to initialize GLFW" << std::endl;
            exit(-1);
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // 2. 召唤梦子的专属视口
        Handle = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!Handle) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(-1);
        }
        glfwMakeContextCurrent(Handle);
        glfwSwapInterval(1);

        // 3. 强行囚禁鼠标！第一人称必须死死锁死
        glfwSetInputMode(Handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // 4. 加载 GLAD 驱动指针地址，连通底层 gl 函数
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            exit(-1);
        }

        // 5. 开启默认的 3D 渲染防御结界
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glfwSetWindowUserPointer(Handle, this);
    }

    // 析构函数：窗口被扬了的时候，自动清理现场，绝不拖泥带水
    ~Window() {
        glfwTerminate();
    }

    //【高雅接口一】向外汇报窗口该不该关闭（用来驱动 main 循环）
    bool ShouldClose() const {
        return glfwWindowShouldClose(Handle);
    }

    //【高雅接口二】强行交换前后帧缓冲区，让显卡吐出画好的草地像素
    void SwapBuffers() {
        glfwSwapBuffers(Handle);
    }

    //【高雅接口三】动态向底层讨要对齐了 DPI 缩放的【真实物理像素尺寸】
    void GetFramebufferSize(int* width, int* height) const {
        glfwGetFramebufferSize(Handle, width, height);
    }

    //【高雅接口四】完美内聚的 F11 全屏切换防抽搐法阵！
    void ToggleFullscreen() {

        //std::cout<< "Toggle! "<< IsFullscreen<< std::endl;

        // 完美的全局记忆步调对齐
        if (!IsFullscreen) {
            glfwGetWindowPos(Handle, &WindowedX, &WindowedY);
            glfwGetWindowSize(Handle, &WindowedWidth, &WindowedHeight);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            glfwSetWindowMonitor(Handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            IsFullscreen = true;

            return; // 现场施法完毕，强制立即退场，不给下面按键同步乱动的时间，哼唧！
        }
        else {
            // 顺着记忆碎片，把窗口完美丢回它原本的大小和桌面坐标！
            glfwSetWindowMonitor(Handle, NULL, WindowedX, WindowedY, WindowedWidth, WindowedHeight, 0);
            IsFullscreen = false;

            return;
        }
    }
};

#endif
