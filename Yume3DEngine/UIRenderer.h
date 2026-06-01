#pragma once

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <iostream>
#include <memory>
#include "Shader.h" // 完美复用基类骨架

class UIRenderer {
private:
    unsigned int uiVAO, uiVBO;       // 管道 A：2D 贴图模板
    unsigned int lineVAO, lineVBO;   // 管道 B：纯数学十字线模板
    std::unique_ptr<Shader> uiShader;

public:
    UIRenderer() : uiVAO(0), uiVBO(0), lineVAO(0), lineVBO(0), uiShader(nullptr) {
        // 梦子看这里！全宇宙最帅气、100% 防踩空的【真·2D零文件内嵌着色器法阵】！
        // 用 R"glsl(...)glsl" 把通用 2D 顶点源码死死焊在 C++ 的肉体里，休想再去硬盘里翻箱倒柜了，哼唧！
        const char* inline2DVS = R"glsl(
            #version 330 core
            layout (location = 0) in vec4 vertex; 
            out vec2 TexCoords;
            uniform mat4 projection2D;
            uniform vec2 uiPosition;
            uniform vec2 uiSize;
            uniform float isLine; 
            void main() {
                vec2 finalPixelPos;
                if (isLine > 0.5) {
                    finalPixelPos = vertex.xy; // 划线模式：直接吃绝对像素
                } else {
                    finalPixelPos = uiPosition + (vertex.xy * uiSize); // 贴图模式：大展开
                }
                gl_Position = projection2D * vec4(finalPixelPos, 0.0, 1.0);
                TexCoords = vertex.zw;
            }
        )glsl";

        // 把通用 Alpha 镂空与纯数学白色准星片段源码也狠狠焊死在内存最底层！
        const char* inline2DFS = R"glsl(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoords;
            uniform sampler2D texture1;
            uniform float isLine;
            void main() {
                if (isLine > 0.5) {
                    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // 亮起梦子最爱的 2.5 像素纯白十字！
                } else {
                    vec4 sampledColor = texture(texture1, TexCoords);
                    if (sampledColor.a < 0.1) discard;
                    FragColor = sampledColor;
                }
            }
        )glsl";

        //  纯内存编译，一纳秒瞬发生成最权威的 2D 渲染主权 programID ！
        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &inline2DVS, NULL); glCompileShader(vs);
        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &inline2DFS, NULL); glCompileShader(fs);
        unsigned int programID = glCreateProgram();
        glAttachShader(programID, vs); glAttachShader(programID, fs); glLinkProgram(programID);
        glDeleteShader(vs); glDeleteShader(fs);

        // 套壳万能基类，并用这个绝对成功的合法 programID 强行接管主权！彻底杜绝 0 路径反噬！
        uiShader = std::make_unique<Shader>("", "");
        uiShader->ID = programID;

        // ---  管道 A：给普通 2D 贴图准备的 1x1 正方形平面模板 ---
        float quadVertices[] = {
            0.0f, 1.0f, 0.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f, 0.0f
        };
        glGenVertexArrays(1, &uiVAO); glGenBuffers(1, &uiVBO);
        glBindVertexArray(uiVAO); glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        // ---  管道 B：完美捕获梦子 4 点配对的绝对像素十字连线骨架！ ---
        // 对齐你最舒服的 2.5 像素风准星大小！
        float crosshairVertices[] = {
             0.0f,  12.0f,  0.0f, 0.0f, // 竖线上点
             0.0f, -12.0f,  0.0f, 0.0f, // 竖线下点
           -12.0f,   0.0f,  0.0f, 0.0f, // 横线左点
            12.0f,   0.0f,  0.0f, 0.0f  // 横线右点
        };
        glGenVertexArrays(1, &lineVAO); glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO); glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    ~UIRenderer() {
        if (uiVAO != 0) glDeleteVertexArrays(1, &uiVAO); if (uiVBO != 0) glDeleteBuffers(1, &uiVBO);
        if (lineVAO != 0) glDeleteVertexArrays(1, &lineVAO); if (lineVBO != 0) glDeleteBuffers(1, &lineVBO);
    }

    //  招式一：梦子最自豪的参数化十字准星大招！
    void DrawCrosshair(float screenWidth, float screenHeight) {
        glDisable(GL_DEPTH_TEST);
        uiShader->use();

        glUniform1f(glGetUniformLocation(uiShader->ID, "isLine"), 1.0f);

        glm::mat4 projection = glm::ortho(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
        uiShader->setMat4("projection2D", projection);
        glUniform2f(glGetUniformLocation(uiShader->ID, "uiPosition"), screenWidth / 2.0f, screenHeight / 2.0f);

        glBindVertexArray(lineVAO);
        glLineWidth(2.5f);
        glDrawArrays(GL_LINES, 0, 4);

        //  全量关闭通道 0 ，完璧归赵还给原始宇宙，防止反噬 3D ！
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

    // 招式二：通用 2D 贴图大招 
    void DrawTexture2D(unsigned int textureID, float x, float y, float width, float height, float screenWidth, float screenHeight) {
        uiShader->use();
        glUniform1f(glGetUniformLocation(uiShader->ID, "isLine"), 0.0f);

        glm::mat4 projection = glm::ortho(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
        uiShader->setMat4("projection2D", projection);
        glUniform2f(glGetUniformLocation(uiShader->ID, "uiPosition"), x, y);
        glUniform2f(glGetUniformLocation(uiShader->ID, "uiSize"), width, height);

        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, textureID);

        glBindVertexArray(uiVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(0); // 干净清零

        glBindVertexArray(0); glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    }
};

#endif
