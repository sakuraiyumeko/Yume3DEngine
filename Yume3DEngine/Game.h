#pragma once

#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <memory>
#include <glm/glm.hpp>
#include "Window.h"
#include "Input.h"
#include "camera.h"
#include "World.h"
#include "Shader.h"
#include "Texture.h"
#include "Renderer.h"
#include "Interaction.h"
#include "Physics.h"

class Game {
private:
    // 梦子你看，这些是我们的“家当”
    std::unique_ptr<Window> gameWindow;
    std::unique_ptr<Shader> ourShader;
    std::unique_ptr<Texture> texture;
    std::unique_ptr<World> world;         // 大写的 World 是类，小写的 world 是变量名！
    std::unique_ptr<Renderer> worldRenderer;
    std::unique_ptr<Camera> camera;

    BlockType currentSelectBlock = BlockType::GRASS;

    unsigned int VAO;
    bool IsN = false;
    bool isSprintLocked = false;
    float interactionTimer = 0.0f;

public:
    // 构造函数：把箱子先摆好
    Game() : VAO(0), IsN(false), isSprintLocked(false), interactionTimer(0.0f), gameWindow(nullptr), currentSelectBlock(BlockType::GRASS), ourShader(nullptr),
        texture(nullptr), world(nullptr),
        worldRenderer(nullptr), camera(nullptr) {
    }

    ~Game() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    }

    void Init(int width, int height, const char* title) {
        gameWindow = std::make_unique<Window>(width, height, title);

        // 1. 世界诞生
        world = std::make_unique<World>();
        world->GenerateWorld(1);

        // 2. 资源装载
        ourShader = std::make_unique<Shader>("shader.vs", "shader.fs");
        texture = std::make_unique<Texture>("texture.jpg", GL_RGB);

        // 现代 OpenGL 必须绑定一个 VAO 才能执行 Draw 核心指令
        // 我们只在显存里给它注册一个空名字，根本不用往里面灌任何普通顶点数据！
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        // 只要解绑就好啦！剩下的 instanceVBO 管道会在 Chunk::Render 里面动态挂接在通道 2 上！
        glBindVertexArray(0);

        // 3. 玩家降临 (177cm 的梦子出发！)
        
        glfwSetInputMode(gameWindow->Handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(gameWindow->Handle, Input::MouseCallback);

        // 初始位置稍微抬高到 (8, 64, 8)，安全踩在你的平原上
        camera = std::make_unique<Camera>(glm::vec3(8.0f, 65.0f, 8.0f));
        worldRenderer = std::make_unique<Renderer>();
    }

    void RunLoop() {
        // 游戏开局前更新一次，保持清爽
        Input::Update();

        while (!gameWindow->ShouldClose()) {
            float dt = Input::GetDeltaTime();
            if (dt > 0.033f) dt = 0.033f;

            // 1. 梦子听好了！必须先让 GLFW 从操作系统手里把鼠标的最新坐标抓过来！
            glfwPollEvents();

            // 2. 然后，再呼唤你的输入系统去计算这一帧的坐标偏移量！
            Input::Update();

            // 3. 接下来再去快乐地移动和渲染！
            update(dt);
            render();

            gameWindow->SwapBuffers();
        }
    }

private:
    void update(float dt) {

        GLFWwindow* handle = gameWindow->Handle;

        if (Input::IsKeyPressed(handle, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(handle, true);
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_F11)) gameWindow->ToggleFullscreen();
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_N)) IsN = !IsN;

        if (IsN) {
            if (Input::IsKeyPressed(handle, GLFW_KEY_SPACE)) { camera->Position.y += 15.0f * dt; }
            if (Input::IsKeyPressed(handle, GLFW_KEY_LEFT_SHIFT)) { camera->Position.y -= 15.0f * dt; }
            if (camera->Zoom != 60.0f) { camera->Zoom += (60.0f - camera->Zoom) * 10.0f * dt; }
        }
        else {
            if (Input::IsKeyPressed(handle, GLFW_KEY_SPACE) && camera->IsGrounded) {
                camera->Velocity.y = camera->JumpForce;
                camera->IsGrounded = false;
            }

            bool isCrouching = !IsN && Input::IsKeyPressed(handle, GLFW_KEY_LEFT_SHIFT);
            bool isMovingForward = Input::IsKeyPressed(handle, GLFW_KEY_W);

            if (Input::IsKeyJustPressed(handle, GLFW_KEY_LEFT_CONTROL) && isMovingForward && !isCrouching) {
                isSprintLocked = true;
            }
            // 只要你手一松开 W 键，或者强行按住了 Shift 下蹲，冲刺状态就会清爽地熄灭！
            if (!isMovingForward || isCrouching) {
                isSprintLocked = false;
            }

            float targetEyeHeight = isCrouching ? 1.2f : 1.7f;

            float targetSpeed = 5.0f; // 默认你原汁原味的日常走位 5.0f 移速
            if (isCrouching) {
                targetSpeed = 2.0f;
            }
            else if (isSprintLocked) {
                // 轰！！！只要冲刺锁处于激活状态，这一帧全方向（无论是W还是A/D）统统都是 32.0f 的狂暴速度！
                targetSpeed = 7.0f;
            }

            float targetFov = isSprintLocked ? 75.0f : 60.0f;

            // 头顶防撞检测（保持你完美的判定不变）
            if (camera->EyeHeight == 1.2f && targetEyeHeight == 1.7f) {
                glm::vec3 testPos = camera->Position;
                testPos.y += (1.7f - 1.2f);
                if (Physics::CheckCollision(testPos, camera.get(), world.get())) {
                    targetEyeHeight = 1.2f;
                    targetSpeed = 2.0f;
                }
            }

            if (camera->EyeHeight != targetEyeHeight) {
                float heightDiff = targetEyeHeight - camera->EyeHeight;
                camera->Position.y += heightDiff;
                camera->EyeHeight = targetEyeHeight;
            }

            //  将统一算好的 targetSpeed 注入相机内部！这样接下来不论执行哪个 ProcessKeyboard，统统共享这一个移速！
            camera->MovementSpeed = targetSpeed;

            if (camera->Zoom != targetFov) {
                camera->Zoom += (targetFov - camera->Zoom) * 10.0f * dt;
            }
        }

        // 1. 在调用 4 个方向之前，先把相机的水平速度方向彻底清零
        camera->HorizontalVelocity = glm::vec3(0.0f);

        // 2. 依次进入 4 个通道，此时 HorizontalVelocity 里面装的是纯粹的、没有被速度污染的 1.0 长度的方向叠加！
        if (Input::IsKeyPressed(handle, GLFW_KEY_W)) camera->ProcessKeyboard(1, dt);
        if (Input::IsKeyPressed(handle, GLFW_KEY_S)) camera->ProcessKeyboard(2, dt);
        if (Input::IsKeyPressed(handle, GLFW_KEY_A)) camera->ProcessKeyboard(3, dt);
        if (Input::IsKeyPressed(handle, GLFW_KEY_D)) camera->ProcessKeyboard(4, dt);

        // 3. 轰！！！当 4 个按键全部叠加完毕的这一瞬间，我们在最外面执行唯一一次神圣的归一化约束！
        // 这样 W 的方向向量（1.0）和 D 的方向向量（1.0）就能绝对平权，两轴长度完全相等！
        if (glm::length(camera->HorizontalVelocity) > 0.0f) {
            // 削减斜边超速，并且一口气注入你上面控死的 32.0f 或者 5.0f 统一度数！
            camera->HorizontalVelocity = glm::normalize(camera->HorizontalVelocity) * camera->MovementSpeed;
        }

        world->UpdateInfiniteWorld(camera->Position, 2);

        processPhysics(dt);
        processInteraction(handle);

        glm::vec2 mouseOffset = Input::GetMouseOffset();
        camera->ProcessMouseMovement(mouseOffset.x, mouseOffset.y);

        if (Input::IsKeyJustPressed(handle, GLFW_KEY_1)) { currentSelectBlock = BlockType::GRASS; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_2)) { currentSelectBlock = BlockType::DIRT; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_3)) { currentSelectBlock = BlockType::STONE; }
    }



    void processPhysics(float dt) {

        if (IsN) {
            camera->Velocity.y = 0.0f;
            camera->IsGrounded = false;
            camera->Gravity = 0.0f;
            camera->MovementSpeed = 15.0f;
            camera->Position.x += camera->HorizontalVelocity.x * dt;
            camera->Position.z += camera->HorizontalVelocity.z * dt;
        }
        else {
            camera->Gravity = -20.0f;
            camera->MovementSpeed = 5.0f;
        }

        if (!camera->IsGrounded) camera->Velocity.y += camera->Gravity * dt;
        else camera->Velocity.y = 0.0f;

        glm::vec3 oldPos = camera->Position;

        camera->Position.x += camera->HorizontalVelocity.x * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            camera->Position.x = oldPos.x;
        }

        camera->Position.z += camera->HorizontalVelocity.z * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            camera->Position.z = oldPos.z;
        }

        camera->Position.y += camera->Velocity.y * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            if (camera->Velocity.y < 0) {
                camera->IsGrounded = true;
                camera->Velocity.y = 0.0f;
                // 稳稳地停留在方块顶端，不多不少，严丝合缝！
                float groundLevel = floor(camera->Position.y - camera->EyeHeight);
                camera->Position.y = groundLevel + 1.0f + camera->EyeHeight;
            }
            else {
                camera->Velocity.y = 0.0f;
                camera->Position.y = oldPos.y;
            }
        }
        else {
            glm::vec3 probe = camera->Position;
            probe.y -= 0.1f;
            if (!Physics::CheckCollision(probe, camera.get(), world.get())) camera->IsGrounded = false;
        }
    }

    void processInteraction(GLFWwindow* handle) {
        
        static bool leftLast = false, rightLast = false;

        float dt = Input::GetDeltaTime();

        if (interactionTimer > 0.0f) {
            interactionTimer -= dt;
        }

        bool leftPress = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightPress = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        bool canLeftInteract = (leftPress && !leftLast) || (leftPress && interactionTimer <= 0.0f);
        bool canRightInteract = (rightPress && !rightLast) || (rightPress && interactionTimer <= 0.0f);

        if (canLeftInteract || canRightInteract) {
            auto result = Interaction::CastRay(camera.get(), world.get());
            if (result.hit) {
                // 1. 梦子听好了！如果是左键，目标就是被击中的那个方块
                glm::ivec3 targetPos = result.pos;

                // 2. 如果是右键，目标就顺着法线往前推一格（也就是要生成的空气格位置）
                if (rightPress) {
                    targetPos += result.normal;
                }

                //  核心修复：不管左键右键，拿到了最终的绝对 targetPos 之后，再来计算区块和局部坐标！
                int cx = (int)floor((float)targetPos.x / 16.0f);
                int cz = (int)floor((float)targetPos.z / 16.0f);

                int lx = targetPos.x % 16; if (lx < 0) lx += 16;
                int lz = targetPos.z % 16; if (lz < 0) lz += 16;

                ChunkPos cp = { cx, cz };
                if (world->chunks.count(cp)) {

                    if (leftPress) {
                        world->chunks[cp]->SetBlock(lx, targetPos.y, lz, BlockType::AIR);

                        // 重点：只要这一帧成功刨开了一个方块，立刻把计时器拉回 0.15 秒！
                        interactionTimer = 0.15f;
                    }

                    else if (rightPress) {
                        // 1. 定义梦子的“身体半径”（比如 0.3f，给你的 8 头身留点空隙）
                        float radius = 0.3f;

                        // 2. 算出梦子身体占据的整数坐标范围（X 和 Z 轴）
                        int minX = (int)floor(camera->Position.x - radius);
                        int maxX = (int)floor(camera->Position.x + radius);
                        int minZ = (int)floor(camera->Position.z - radius);
                        int maxZ = (int)floor(camera->Position.z + radius);

                        // 3. 算出高度占据的范围（从脚底到头顶）
                        int minY = (int)floor(camera->Position.y - camera->EyeHeight);
                        int maxY = (int)floor(camera->Position.y + 0.2f); // 给头顶留一点点余量

                        // 4. 开始多重判定！
                        bool isInsidePlayer = (targetPos.x >= minX && targetPos.x <= maxX) &&
                            (targetPos.z >= minZ && targetPos.z <= maxZ) &&
                            (targetPos.y >= minY && targetPos.y <= maxY);

                        if (!isInsidePlayer) {
                            world->chunks[cp]->SetBlock(lx, targetPos.y, lz, currentSelectBlock);
                            // ... 剩下的刷新逻辑保持不变 ...
                            interactionTimer = 0.15f;
                        }
                    }

                    // 1. 刷新被修改的这个区块
                    world->UpdateChunkMesh(cp);

                    // 2.  核心修复 2：连带责任刷新！
                    // 如果梦子敲击的是区块的边缘，必须强迫隔壁的区块也重新审视一下自己的面！
                    if (lx == 0)  world->UpdateChunkMesh({ cp.x - 1, cp.z });
                    if (lx == 15) world->UpdateChunkMesh({ cp.x + 1, cp.z });
                    if (lz == 0)  world->UpdateChunkMesh({ cp.x, cp.z - 1 });
                    if (lz == 15) world->UpdateChunkMesh({ cp.x, cp.z + 1 });
                }
            }
        }
        leftLast = leftPress; rightLast = rightPress;
    }

    void render() {
        worldRenderer->DrawScene(*gameWindow, *ourShader, *texture, *camera, world.get(), VAO);
    }
};

#endif