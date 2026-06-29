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
#include "UIRenderer.h"

class Game {
private:
    // 梦子你看，这些是我们的“家当”
    std::unique_ptr<Window> gameWindow;
    std::unique_ptr<Shader> ourShader;        // 3D 方块世界的着色器
    std::unique_ptr<Shader> uiShader;         //  新增：外置的全能通用 UI 着色器！
    std::unique_ptr<Texture> texture;
    std::unique_ptr<World> world;
    std::unique_ptr<Renderer> worldRenderer;
    std::unique_ptr<UIRenderer> ui;
    std::unique_ptr<Camera> camera;

    BlockType currentSelectBlock = BlockType::GRASS;

    unsigned int VAO;
    bool IsN = false;
    bool isSprintLocked = false;
    float interactionTimer = 0.0f;
    float currentvelo = 0;

    float VelocityProcess(float velo) {
        currentvelo = velo;
        currentvelo = 0.9 * currentvelo + 0.1 * velo;
        return currentvelo;
    }

    void processInputStates(GLFWwindow* handle, float dt) {
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_N)) IsN = !IsN;

        bool isMovingForward = Input::IsKeyPressed(handle, GLFW_KEY_W);
        bool isCrouching = !IsN && Input::IsKeyPressed(handle, GLFW_KEY_LEFT_SHIFT); // 只有生存模式才能下蹲

        if (Input::IsKeyJustPressed(handle, GLFW_KEY_LEFT_CONTROL) && isMovingForward && !isCrouching) {
            isSprintLocked = true;
        }
        // 手一松开 W，冲刺立刻熄灭！
        if (!isMovingForward || isCrouching) {
            isSprintLocked = false;
        }

        if (IsN) {
            camera->Gravity = 0.0f;
            camera->Velocity = glm::vec3(0.0f);
            camera->IsGrounded = false;

            //  飞天极限超速魔法：默认飞行 15.0f，一旦开启 LC 冲刺锁，直接狂暴飙到 32.0f（你的极限冲刺速度！）
            float targetSpeed = isSprintLocked ? 20.0f : 10.0f;
            camera->MovementSpeed = targetSpeed;

            // 飞行时的视野拉伸：冲刺时 FOV 放大到 85 度，体验时空撕裂感！
            float targetFov = isSprintLocked ? 100.0f : 75.0f;

            // 梦子注意：飞天时的垂直升降速度也同步享受速度加成噢！
            float verticalSpeed = targetSpeed;
            if (Input::IsKeyPressed(handle, GLFW_KEY_SPACE)) { camera->Position.y += verticalSpeed * dt; }
            if (Input::IsKeyPressed(handle, GLFW_KEY_LEFT_SHIFT)) { camera->Position.y -= verticalSpeed * dt; }

            if (camera->Zoom != targetFov) {
                camera->Zoom += (targetFov - camera->Zoom) * 10.0f * dt;
            }
        }
        else {
            // 普通生存模式的物理法则保持完美不变
            camera->Gravity = -18.0f;

            if (Input::IsKeyPressed(handle, GLFW_KEY_SPACE) && camera->IsGrounded) {
                camera->Velocity.y = camera->JumpForce;
                camera->IsGrounded = false;
            }

            bool isCrouching = !IsN && Input::IsKeyPressed(handle, GLFW_KEY_LEFT_SHIFT);
            bool isMovingForward = Input::IsKeyPressed(handle, GLFW_KEY_W);

            if (Input::IsKeyJustPressed(handle, GLFW_KEY_LEFT_CONTROL) && isMovingForward && !isCrouching) {
                isSprintLocked = true;
            }
            if (!isMovingForward || isCrouching) {
                isSprintLocked = false;
            }

            float targetEyeHeight = isCrouching ? 1.5f : 1.62f;
            float targetSpeed = 3.5f;
            if (isCrouching) {
                targetSpeed = 1.5f;
            }
            else if (isSprintLocked) {
                targetSpeed = 5.0f;
            }

            float targetFov = isSprintLocked ? 95.0f : 75.0f;

            if (camera->EyeHeight == 1.5f && targetEyeHeight == 1.62f) {
                glm::vec3 testPos = camera->Position;
                testPos.y += (1.62f - 1.5f);
                if (Physics::CheckCollision(testPos, camera.get(), world.get())) {
                    targetEyeHeight = 1.5f;
                    targetSpeed = 2.0f;
                }
            }

            if (camera->EyeHeight != targetEyeHeight) {
                float heightDiff = targetEyeHeight - camera->EyeHeight;
                camera->Position.y += heightDiff;
                camera->EyeHeight = targetEyeHeight;
            }

            camera->MovementSpeed = targetSpeed;

            if (camera->Zoom != targetFov) {
                camera->Zoom += (targetFov - camera->Zoom) * 10.0f * dt;
            }
        }
    }

    //  拆出来的子模块二：专门搞键盘 4 方向的叠加向量和归一化防斜向超速
    void processMovementVectors(GLFWwindow* handle, float dt) {
        // 每一帧开头，先把上一帧的移动意图清空
        camera->WishDir = glm::vec3(0.0f);

        // 收集 W A S D 的方向
        if (Input::IsKeyPressed(handle, GLFW_KEY_W)) camera->ProcessKeyboard(1);
        if (Input::IsKeyPressed(handle, GLFW_KEY_S)) camera->ProcessKeyboard(2);
        if (Input::IsKeyPressed(handle, GLFW_KEY_A)) camera->ProcessKeyboard(3);
        if (Input::IsKeyPressed(handle, GLFW_KEY_D)) camera->ProcessKeyboard(4);

        // 如果有按键按下，执行归一化，彻底防范斜向走超速
        if (glm::length(camera->WishDir) > 0.0f) {
            camera->WishDir = glm::normalize(camera->WishDir);
        }
    }

    //  拆出来的子模块三：方块快捷栏选定
    void updateItemSelection(GLFWwindow* handle) {
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_1)) { currentSelectBlock = BlockType::GRASS; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_2)) { currentSelectBlock = BlockType::DIRT; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_3)) { currentSelectBlock = BlockType::STONE; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_2)) { currentSelectBlock = BlockType::LOG; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_3)) { currentSelectBlock = BlockType::LEAVES; }
    }

public:
    // 构造函数：把箱子先摆好
    Game() : VAO(0), IsN(false), isSprintLocked(false), interactionTimer(0.0f), gameWindow(nullptr), currentSelectBlock(BlockType::GRASS), ourShader(nullptr),
        texture(nullptr), world(nullptr),
        worldRenderer(nullptr), camera(nullptr), ui(nullptr) {
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
        ourShader = std::make_unique<Shader>("World.vs", "World.fs");
        uiShader = std::make_unique<Shader>("UI.vs", "UI.fs");
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
        camera = std::make_unique<Camera>(glm::vec3(8.0f, 45.0f, 8.0f));
        worldRenderer = std::make_unique<Renderer>();
        ui = std::make_unique<UIRenderer>();
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
        // 0. 帧率安全限制锁
        if (dt > 0.03f) dt = 0.03f;
        GLFWwindow* handle = gameWindow->Handle;

        // 1. 系统底层级按键直接拦截
        if (Input::IsKeyPressed(handle, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(handle, true);
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_F11)) gameWindow->ToggleFullscreen();

        //  核心解耦大招一：处理错综复杂的运动状态机（下蹲、疾跑锁、FOV过渡）
        processInputStates(handle, dt);

        //  核心解耦大招二：计算 WASD 水平运动向量并执行神圣的归一化
        processMovementVectors(handle, dt);

        // 3. 大世界无限区块刷新魔法
        world->UpdateInfiniteWorld(camera->Position, 5);

        // 4. 派发给外部独立的物理和交互模块
        processPhysics(dt);
        processInteraction(handle);

        // 5. 视角旋转矩阵计算
        glm::vec2 mouseOffset = Input::GetMouseOffset(handle);
        camera->ProcessMouseMovement(mouseOffset.x, mouseOffset.y);

        //  核心解耦大招三：处理数字键快捷栏方块切换
        updateItemSelection(handle);
    }

    void processPhysics(float dt) {
        if (IsN) {
            camera->CurrentFriction = 2.0f;
        }
        else if (camera->IsGrounded) {
            glm::vec3 footBlockPos = camera->Position;
            footBlockPos.y -= (camera->EyeHeight + 0.1f);
            BlockType standingBlock = world->GetBlockAt(footBlockPos);

            //if (standingBlock == BlockType::AIR) {
            //    camera->CurrentFriction = 0.5f; // 冰面允许大幅度打滑
            //}
            //else if (standingBlock == BlockType::SOUL_SAND) {
            //    camera->CurrentFriction = 18.0f; // 瞬间粘住
            //}
            //else {
                camera->CurrentFriction = camera->DEFAULT_FRICTION; // 普通地面默认 8.0f 
            //}
        }
        else {
            camera->CurrentFriction = 1.0f; // 空中保留更多身法惯性
        }

        // --- 1. 【核心修复】：Quake / Minecraft 式投影加速与分轴阻尼 ---
        glm::vec3 curVel = camera->HorizontalVelocity;
        float wishSpeed = camera->MovementSpeed;

        if (glm::length(camera->WishDir) > 0.001f) {
            glm::vec3 wishDir = glm::normalize(camera->WishDir);

            // 【消除转向打滑的关键】：计算当前速度在玩家“移动意图方向”上的投影
            float currentSpeedInWishDir = glm::dot(curVel, wishDir);

            // 计算这一帧还能往这个方向追加多少速度
            float addSpeed = wishSpeed - currentSpeedInWishDir;

            if (addSpeed > 0.0f) {
                // 根据按键方向强行追加速度，数值越大起步和转向越灵敏
                float accelSpeed = camera->ACCELERATION * dt * wishSpeed;
                if (accelSpeed > addSpeed) accelSpeed = addSpeed;
                curVel += wishDir * accelSpeed;
            }

            // 对于“垂直于移动意图”的侧向惯性残留，实行强力的摩擦衰减，防止严重的横向漂移
            glm::vec3 sideVel = curVel - wishDir * glm::dot(curVel, wishDir);
            if (glm::length(sideVel) > 0.001f) {
                // 普通方块上侧向刹车极快，冰面上侧向刹车慢（保留漂移）
                curVel -= sideVel * (1.0f - glm::exp(-camera->CurrentFriction * 1.5f * dt));
            }
        }
        else {
            // 【完全松开按键】：全轴执行平滑的摩擦力自然衰减
            if (glm::length(curVel) > 0.001f) {
                curVel *= glm::exp(-camera->CurrentFriction * dt);
            }
            else {
                curVel = glm::vec3(0.0f);
            }
        }

        camera->HorizontalVelocity = curVel;

        // --- 2. 速度限幅（动态适配你在 processInputStates 里算好的各种状态速） ---
        float speed = glm::length(camera->HorizontalVelocity);
        if (speed > camera->MovementSpeed) {
            camera->HorizontalVelocity = glm::normalize(camera->HorizontalVelocity) * camera->MovementSpeed;
        }

        // 穿墙模式直接潇洒离场
        if (IsN) {
            camera->Position.x += camera->HorizontalVelocity.x * dt;
            camera->Position.z += camera->HorizontalVelocity.z * dt;
            return;
        }

        // 下面全都是普通生存模式下，梦子 177cm 身高专属的严密 AABB 碰撞检测
        if (!camera->IsGrounded) camera->Velocity.y += camera->Gravity * dt;
        else camera->Velocity.y = 0.0f;

        glm::vec3 oldPos = camera->Position;

        // X 轴移动与碰撞
        camera->Position.x += camera->HorizontalVelocity.x * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            camera->Position.x = oldPos.x;
            camera->HorizontalVelocity.x = 0.0f; // 撞墙清除 X 轴速度分量
        }

        // Z 轴移动与碰撞
        camera->Position.z += camera->HorizontalVelocity.z * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            camera->Position.z = oldPos.z;
            camera->HorizontalVelocity.z = 0.0f; // 撞墙清除 Z 轴速度分量
        }

        // Y 轴移动与碰撞
        camera->Position.y += camera->Velocity.y * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            if (camera->Velocity.y < 0) {
                camera->IsGrounded = true;
                camera->Velocity.y = 0.0f;
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
                glm::ivec3 targetPos = result.pos;

                if (rightPress) {
                    targetPos += result.normal;
                }

                int cx = (int)floor((float)targetPos.x / 16.0f);
                int cz = (int)floor((float)targetPos.z / 16.0f);

                int lx = targetPos.x % 16; if (lx < 0) lx += 16;
                int lz = targetPos.z % 16; if (lz < 0) lz += 16;

                ChunkPos cp = { cx, cz };
                if (world->chunks.count(cp)) {
                    bool blockChanged = false;

                    if (leftPress) {
                        world->chunks[cp]->SetBlock(lx, targetPos.y, lz, BlockType::AIR);
                        interactionTimer = 0.15f;
                        blockChanged = true;
                    }
                    else if (rightPress) {
                        // 梦子大长腿与身体的防窒息碰撞盒检查
                        float radius = 0.3f;
                        int minX = (int)floor(camera->Position.x - radius);
                        int maxX = (int)floor(camera->Position.x + radius);
                        int minZ = (int)floor(camera->Position.z - radius);
                        int maxZ = (int)floor(camera->Position.z + radius);
                        int minY = (int)floor(camera->Position.y - camera->EyeHeight);
                        int maxY = (int)floor(camera->Position.y + 0.2f);

                        bool isInsidePlayer = (targetPos.x >= minX && targetPos.x <= maxX) &&
                            (targetPos.z >= minZ && targetPos.z <= maxZ) &&
                            (targetPos.y >= minY && targetPos.y <= maxY);

                        if (!isInsidePlayer) {
                            world->chunks[cp]->SetBlock(lx, targetPos.y, lz, currentSelectBlock);
                            interactionTimer = 0.15f;
                            blockChanged = true;
                        }
                    }

                    //  终极性能优化：如果方块真的改变了，绝不在主线程当场死等硬重绘！
                    if (blockChanged) {
                        // 1. 自己这个区块，一秒钟都不许等，当场硬熔炼！
                        world->ForceUpdateInteractionMesh(cp);

                        // 2. 邻居区块如果有连带责任，同样在这一帧当场顺发熔炼！再也没有任何时间差！
                        if (lx == 0 && world->chunks.count({ cp.x - 1, cp.z })) world->ForceUpdateInteractionMesh({ cp.x - 1, cp.z });
                        if (lx == 15 && world->chunks.count({ cp.x + 1, cp.z })) world->ForceUpdateInteractionMesh({ cp.x + 1, cp.z });
                        if (lz == 0 && world->chunks.count({ cp.x, cp.z - 1 })) world->ForceUpdateInteractionMesh({ cp.x, cp.z - 1 });
                        if (lz == 15 && world->chunks.count({ cp.x, cp.z + 1 })) world->ForceUpdateInteractionMesh({ cp.x, cp.z + 1 });
                    }
                }
            }
        }
        leftLast = leftPress; rightLast = rightPress;
    }

    void render() {
        worldRenderer->DrawScene(*gameWindow, *ourShader, *texture, *camera, world.get(), VAO);
        ui->DrawCrosshair(*uiShader); // 白嫖全能着色器画准星！
        ui->DrawRect(*uiShader, glm::vec2(-0.4f, -0.9f), glm::vec2(0.8f, 0.12f), glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));
    }
};

#endif