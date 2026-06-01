#pragma once

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include "Shader.h"
#include "Texture.h" 

class UIRenderer {
private:
    //  终极瘦身：准星和UI的着色器在外面合二为一了！这里只剩纯粹的 2D 硬件网格
    unsigned int crosshairVAO, crosshairVBO;
    unsigned int quadVAO, quadVBO;

public:
    UIRenderer() : crosshairVAO(0), crosshairVBO(0), quadVAO(0), quadVBO(0) {
        // 1. 初始化十字准星网格（纯顶点位置）
        float crosshairVertices[] = {
             0.00f,  0.03f,   0.00f, -0.03f,
            -0.017f, 0.00f,   0.017f, 0.00f
        };
        glGenVertexArrays(1, &crosshairVAO);
        glGenBuffers(1, &crosshairVBO);
        glBindVertexArray(crosshairVAO);
        glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        // 2. 初始化全能归一化平面网格（位置 + UV）
        float quadVertices[] = {
            0.0f, 1.0f,  0.0f, 1.0f,
            0.0f, 0.0f,  0.0f, 0.0f,
            1.0f, 0.0f,  1.0f, 0.0f,

            0.0f, 1.0f,  0.0f, 1.0f,
            1.0f, 0.0f,  1.0f, 0.0f,
            1.0f, 1.0f,  1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    ~UIRenderer() {
        if (crosshairVAO != 0) glDeleteVertexArrays(1, &crosshairVAO);
        if (crosshairVBO != 0) glDeleteBuffers(1, &crosshairVBO);
        if (quadVAO != 0) glDeleteVertexArrays(1, &quadVAO);
        if (quadVBO != 0) glDeleteBuffers(1, &quadVBO);
    }

    //  1. 终极合并：准星直接白嫖全能 UI 着色器！
    void DrawCrosshair(Shader& uiShader) {
        glDisable(GL_DEPTH_TEST);
        uiShader.use();

        // 告诉全能着色器：现在不需要贴图，直接给老娘渲染纯白色的准星线段！
        uiShader.setInt("useTexture", 0);
        uiShader.setVec4("uiColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // 纯正白准星

        // 因为你的准星顶点已经是算好的NDC坐标了，所以矩阵直接给单位矩阵，不做任何空间拉伸！
        uiShader.setMat4("model", glm::mat4(1.0f));

        glBindVertexArray(crosshairVAO);
        glLineWidth(2.5f);
        glDrawArrays(GL_LINES, 0, 4); // 轰！！！4个点用全能着色器连成纯白十字！
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

    // 2. 通用纯色几何绘制（依然使用同一个 uiShader）
    void DrawRect(Shader& uiShader, glm::vec2 position, glm::vec2 size, glm::vec4 color) {
        glDisable(GL_DEPTH_TEST);

        //  核心大修复：唤醒沉睡的 Alpha 透明混合魔法阵！
        glEnable(GL_BLEND);
        // 设定混合公式：让 0.6 的黑和原版天空的蓝色进行完美的赛博朋克物理揉和！
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        uiShader.use();
        uiShader.setInt("useTexture", 0);
        uiShader.setVec4("uiColor", color);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::scale(model, glm::vec3(size, 1.0f));
        uiShader.setMat4("model", model);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        //  渲染完 UI 记得把混合关掉，防止污染你高贵的 3D 世界体素方块哦！
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    //  3. 通用贴图绘制（依然使用同一个 uiShader）
    void DrawTexture(Shader& uiShader, Texture& texture, glm::vec2 position, glm::vec2 size) {
        glDisable(GL_DEPTH_TEST);
        uiShader.use();

        uiShader.setInt("useTexture", 1);
        uiShader.setVec4("uiColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        texture.use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::scale(model, glm::vec3(size, 1.0f));
        uiShader.setMat4("model", model);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
};

#endif
