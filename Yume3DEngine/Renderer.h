#pragma once

#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include "Window.h"
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "Chunk.h"
#include "World.h" //  梦子，高贵的魔导书必须包含进来！

class Renderer {
private:
    std::unique_ptr<Shader> crosshairShader;
    unsigned int uiVAO, uiVBO;

public:
    Renderer() : uiVAO(0), uiVBO(0) {
        crosshairShader = std::make_unique<Shader>("crosshair.vs", "crosshair.fs");

        //  修正：4个点两两配对，一共 8 个 float，完美拉出纯正的十字！
        float crosshairVertices[] = {
             0.00f,  0.03f,   0.00f, -0.03f, // 竖线：点1(0, 0.03) 到 点2(0, -0.03)
            -0.017f, 0.00f,   0.017f, 0.00f  // 横线：点3(-0.017, 0) 到 点4(0.017, 0)
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

    ~Renderer() {
        if (uiVAO != 0) glDeleteVertexArrays(1, &uiVAO);
        if (uiVBO != 0) glDeleteBuffers(1, &uiVBO);
    }

    // 梦子注意：最后的 unsigned int VAO 参数目前用不到了，可以删掉或者留着
    void DrawScene(Window& window, Shader& shader, Texture& texture, Camera& camera, World* world, unsigned int VAO) {

        int displayWidth, displayHeight;
        window.GetFramebufferSize(&displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);

        glClearColor(0.6f, 0.6f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        texture.use();

        glm::mat4 view = camera.GetViewMatrix();
        // 梦子注意：因为我们要看很多区块，把远裁剪面从 100.0f 扩展到 300.0f，防止视线穿模！
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)displayWidth / (float)displayHeight, 0.1f, 300.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // 核心大修复：遍历大世界里的每一个区块，根据它们高贵的 ChunkPos 动态计算平移矩阵！
        for (auto const& [pos, chunk] : world->chunks) {
            glm::mat4 model = glm::mat4(1.0f);
            // 区块的 X 和 Z 坐标必须乘以 16.0f，才是它在 3D 世界中的真正始发点！
            model = glm::translate(model, glm::vec3(pos.x * 16.0f, 0.0f, pos.z * 16.0f));
            shader.setMat4("model", model);

            // 挥下画笔，把这个区块内部的所有实例方块一口气画出来！
            chunk->Render(VAO);
        }

        glDisable(GL_DEPTH_TEST);
        crosshairShader->use();
        glBindVertexArray(uiVAO);

        glLineWidth(2.5f);
        glDrawArrays(GL_LINES, 0, 4); // 轰！！！这下 4 个点能正确连成两根十字线了！

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
};

#endif
