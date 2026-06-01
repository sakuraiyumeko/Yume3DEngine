#pragma once

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <glad/glad.h>
#include <memory>
#include "Shader.h"

class UIRenderer {
private:
    std::unique_ptr<Shader> crosshairShader;
    unsigned int uiVAO, uiVBO;

public:
    UIRenderer() : uiVAO(0), uiVBO(0) {
        // 从主渲染器里高贵地搬家过来的准星着色器！
        crosshairShader = std::make_unique<Shader>("crosshair.vs", "crosshair.fs");

        // 完美拉出纯正十字的 8 个 float
        float crosshairVertices[] = {
             0.00f,  0.03f,   0.00f, -0.03f, // 竖线
            -0.017f, 0.00f,   0.017f, 0.00f  // 横线
        };

        glGenVertexArrays(1, &uiVAO);
        glGenBuffers(1, &uiVBO);

        glBindVertexArray(uiVAO);
        glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    ~UIRenderer() {
        if (uiVAO != 0) glDeleteVertexArrays(1, &uiVAO);
        if (uiVBO != 0) glDeleteBuffers(1, &uiVBO);
    }

    // 专属的 UI 渲染魔法，把深度测试关掉才能完美盖在最上层！
    void DrawUI() {
        glDisable(GL_DEPTH_TEST);

        crosshairShader->use();
        glBindVertexArray(uiVAO);

        glLineWidth(2.5f);
        glDrawArrays(GL_LINES, 0, 4); // 轰！！！4个点正确连成十字！

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
};

#endif
