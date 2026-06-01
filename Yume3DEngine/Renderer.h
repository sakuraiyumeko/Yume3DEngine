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
#include "World.h"

class Renderer {
public:
    Renderer() {}
    ~Renderer() {}

    // 哼！高贵的全局 VAO 重新归位，专门传给每一个区块去画它的实例化方块！
    void DrawScene(Window& window, Shader& shader, Texture& texture, Camera& camera, World* world, unsigned int VAO) {

        int displayWidth, displayHeight;
        window.GetFramebufferSize(&displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);

        glClearColor(0.6f, 0.6f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        texture.use();

        glm::mat4 view = camera.GetViewMatrix();
        // 远裁剪面 300.0f 远眺，完美防止地平线穿模！
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)displayWidth / (float)displayHeight, 0.1f, 300.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // 遍历世界里的每一个区块，动态计算它们的平移矩阵
        for (auto const& [pos, chunk] : world->chunks) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pos.x * 16.0f, 0.0f, pos.z * 16.0f));
            shader.setMat4("model", model);

            // 顺畅对接！把全局 VAO 狠狠地塞进 Chunk 内部的实例化渲染魔法里！
            chunk->Render(VAO);
        }
    }
};

#endif
