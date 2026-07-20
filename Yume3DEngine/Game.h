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
    // 姊﹀瓙浣犵湅锛岃繖浜涙槸鎴戜滑鐨勨€滃褰撯€?
    std::unique_ptr<Window> gameWindow;
    std::unique_ptr<Shader> ourShader;        // 3D 鏂瑰潡涓栫晫鐨勭潃鑹插櫒
    std::unique_ptr<Shader> uiShader;         //  鏂板锛氬缃殑鍏ㄨ兘閫氱敤 UI 鐫€鑹插櫒锛?
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
    int worldseed = 0;

    float VelocityProcess(float velo) {
        currentvelo = velo;
        currentvelo = 0.9 * currentvelo + 0.1 * velo;
        return currentvelo;
    }

    void processInputStates(GLFWwindow* handle, float dt) {
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_N)) IsN = !IsN;

        bool isMovingForward = Input::IsKeyPressed(handle, GLFW_KEY_W);
        bool isCrouching = !IsN && Input::IsKeyPressed(handle, GLFW_KEY_LEFT_SHIFT); // 鍙湁鐢熷瓨妯″紡鎵嶈兘涓嬭共

        if (Input::IsKeyJustPressed(handle, GLFW_KEY_LEFT_CONTROL) && isMovingForward && !isCrouching) {
            isSprintLocked = true;
        }
        // 鎵嬩竴鏉惧紑 W锛屽啿鍒虹珛鍒荤唲鐏紒
        if (!isMovingForward || isCrouching) {
            isSprintLocked = false;
        }

        if (IsN) {
            camera->Gravity = 0.0f;
            camera->Velocity = glm::vec3(0.0f);
            camera->IsGrounded = false;

            //  椋炲ぉ鏋侀檺瓒呴€熼瓟娉曪細榛樿椋炶 15.0f锛屼竴鏃﹀紑鍚?LC 鍐插埡閿侊紝鐩存帴鐙傛毚椋欏埌 32.0f锛堜綘鐨勬瀬闄愬啿鍒洪€熷害锛侊級
            float targetSpeed = isSprintLocked ? 20.0f : 10.0f;
            camera->MovementSpeed = targetSpeed;

            // 椋炶鏃剁殑瑙嗛噹鎷変几锛氬啿鍒烘椂 FOV 鏀惧ぇ鍒?85 搴︼紝浣撻獙鏃剁┖鎾曡鎰燂紒
            float targetFov = isSprintLocked ? 100.0f : 75.0f;

            // 姊﹀瓙娉ㄦ剰锛氶澶╂椂鐨勫瀭鐩村崌闄嶉€熷害涔熷悓姝ヤ韩鍙楅€熷害鍔犳垚鍣紒
            float verticalSpeed = targetSpeed;
            if (Input::IsKeyPressed(handle, GLFW_KEY_SPACE)) { camera->Position.y += verticalSpeed * dt; }
            if (Input::IsKeyPressed(handle, GLFW_KEY_LEFT_SHIFT)) { camera->Position.y -= verticalSpeed * dt; }

            if (camera->Zoom != targetFov) {
                camera->Zoom += (targetFov - camera->Zoom) * 10.0f * dt;
            }
        }
        else {
            // 鏅€氱敓瀛樻ā寮忕殑鐗╃悊娉曞垯淇濇寔瀹岀編涓嶅彉
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

    //  鎷嗗嚭鏉ョ殑瀛愭ā鍧椾簩锛氫笓闂ㄦ悶閿洏 4 鏂瑰悜鐨勫彔鍔犲悜閲忓拰褰掍竴鍖栭槻鏂滃悜瓒呴€?
    void processMovementVectors(GLFWwindow* handle, float dt) {
        // 姣忎竴甯у紑澶达紝鍏堟妸涓婁竴甯х殑绉诲姩鎰忓浘娓呯┖
        camera->WishDir = glm::vec3(0.0f);

        // 鏀堕泦 W A S D 鐨勬柟鍚?
        if (Input::IsKeyPressed(handle, GLFW_KEY_W)) camera->ProcessKeyboard(1);
        if (Input::IsKeyPressed(handle, GLFW_KEY_S)) camera->ProcessKeyboard(2);
        if (Input::IsKeyPressed(handle, GLFW_KEY_A)) camera->ProcessKeyboard(3);
        if (Input::IsKeyPressed(handle, GLFW_KEY_D)) camera->ProcessKeyboard(4);

        // 濡傛灉鏈夋寜閿寜涓嬶紝鎵ц褰掍竴鍖栵紝褰诲簳闃茶寖鏂滃悜璧拌秴閫?
        if (glm::length(camera->WishDir) > 0.0f) {
            camera->WishDir = glm::normalize(camera->WishDir);
        }
    }

    //  鎷嗗嚭鏉ョ殑瀛愭ā鍧椾笁锛氭柟鍧楀揩鎹锋爮閫夊畾    //  淇敼锛氶€夋嫨鏂瑰潡
    void updateItemSelection(GLFWwindow* handle) {
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_1)) { currentSelectBlock = BlockType::GRASS; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_2)) { currentSelectBlock = BlockType::DIRT; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_3)) { currentSelectBlock = BlockType::STONE; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_4)) { currentSelectBlock = BlockType::LOG; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_5)) { currentSelectBlock = BlockType::LEAVES; }
        else if (Input::IsKeyJustPressed(handle, GLFW_KEY_6)) { currentSelectBlock = BlockType::GLOWSTONE; }
    }

public:
    // 鏋勯€犲嚱鏁帮細鎶婄瀛愬厛鎽嗗ソ
    Game() : VAO(0), IsN(false), isSprintLocked(false), interactionTimer(0.0f), gameWindow(nullptr), currentSelectBlock(BlockType::GRASS), ourShader(nullptr),
        texture(nullptr), world(nullptr),
        worldRenderer(nullptr), camera(nullptr), ui(nullptr) {
    }

    ~Game() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    }

    void Init(int width, int height, const char* title) {

        int seed = 0;
        printf("Enter a seed: ");
        scanf_s("%d", &seed);
        this->worldseed = seed;
        TerrainNoise::SetSeed(seed);

        gameWindow = std::make_unique<Window>(width, height, title);

        // 1. 涓栫晫璇炵敓
        world = std::make_unique<World>();
        world->GenerateWorld(1);

        // 2. 璧勬簮瑁呰浇
        ourShader = std::make_unique<Shader>("World.vs", "World.fs");
        uiShader = std::make_unique<Shader>("UI.vs", "UI.fs");
        texture = std::make_unique<Texture>("texture.jpg", GL_RGB);

        // 鐜颁唬 OpenGL 蹇呴』缁戝畾涓€涓?VAO 鎵嶈兘鎵ц Draw 鏍稿績鎸囦护
        // 鎴戜滑鍙湪鏄惧瓨閲岀粰瀹冩敞鍐屼竴涓┖鍚嶅瓧锛屾牴鏈笉鐢ㄥ線閲岄潰鐏屼换浣曟櫘閫氶《鐐规暟鎹紒
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        // 鍙瑙ｇ粦灏卞ソ鍟︼紒鍓╀笅鐨?instanceVBO 绠￠亾浼氬湪 Chunk::Render 閲岄潰鍔ㄦ€佹寕鎺ュ湪閫氶亾 2 涓婏紒
        glBindVertexArray(0);

        // 3. 鐜╁闄嶄复 (177cm 鐨勬ⅵ瀛愬嚭鍙戯紒)
        
        glfwSetInputMode(gameWindow->Handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(gameWindow->Handle, Input::MouseCallback);

        // 鍒濆浣嶇疆绋嶅井鎶珮鍒?(8, 64, 8)锛屽畨鍏ㄨ俯鍦ㄤ綘鐨勫钩鍘熶笂
        float spawnX = 8.0f;
        float spawnZ = 8.0f;

        // 瀹岀編鍏嬮殕浣犵殑涓栫晫鍏紡锛?00% 杩樺師鍦拌〃鍥轰綋鏂瑰潡楂樺害
        float nBase = TerrainNoise::GetPerlin(spawnX * 0.025f, spawnZ * 0.025f) * 25.0f;
        float nDetail = TerrainNoise::GetPerlin(spawnX * 0.1f, spawnZ * 0.1f) * 4.0f;
        int surfH = static_cast<int>(40.0f + nBase + nDetail);

        // 杈圭晫瀹夊叏闄愬箙锛岃窡浣犵殑涓栫晫娉曞垯缁濆瀵归綈
        if (surfH < 1) surfH = 1;
        if (surfH >= 256) surfH = 255;

        // 鐢熸€侀槻鍗￠攣锛氬洜涓?(8, 8) 鍦?0 鍙峰尯鍧楃殑涓績锛屾瀬鏄撻珮姒傜巼闀垮嚭姗℃爲锛?
        // 姗℃爲鐨勪富骞蹭竴鑸珮 5 鏍硷紝涓轰簡褰诲簳闃叉姊﹀瓙寮€灞€鍗″湪鏍戝彾鎴栬€呮爲骞查噷绐掓伅锛?
        // 鎴戜滑鍦ㄨ繖閲岀洿鎺ユ妸鍑虹敓楂樺害寰€涓婂畨鍏ㄦ姮楂?6 鏍硷紝浠庣┖涓紭闆呴檷钀斤紝閰嶅悎浣犻珮璐电殑 AABB 纰版挒锛屽ス浼氳嚜鍔ㄥ畬缇庤惤鍦ㄦ爲椤舵垨鑽夊湴涓婏紒
        float safeSpawnY = static_cast<float>(surfH) + 2.0f + 1.62f;

        // 鍔ㄦ€佸嚭鐢熶綅缃紝褰诲簳鍛婂埆鍗″鍦扮嫳锛?
        camera = std::make_unique<Camera>(glm::vec3(spawnX, safeSpawnY, spawnZ));

        worldRenderer = std::make_unique<Renderer>();
        ui = std::make_unique<UIRenderer>();
    }

    void RunLoop() {
        // 娓告垙寮€灞€鍓嶆洿鏂颁竴娆★紝淇濇寔娓呯埥
        Input::Update();

        while (!gameWindow->ShouldClose()) {
            float dt = Input::GetDeltaTime();
            if (dt > 0.033f) dt = 0.033f;

            // 1. 姊﹀瓙鍚ソ浜嗭紒蹇呴』鍏堣 GLFW 浠庢搷浣滅郴缁熸墜閲屾妸榧犳爣鐨勬渶鏂板潗鏍囨姄杩囨潵锛?
            glfwPollEvents();

            // 2. 鐒跺悗锛屽啀鍛煎敜浣犵殑杈撳叆绯荤粺鍘昏绠楄繖涓€甯х殑鍧愭爣鍋忕Щ閲忥紒
            Input::Update();

            // 3. 鎺ヤ笅鏉ュ啀鍘诲揩涔愬湴绉诲姩鍜屾覆鏌擄紒
            update(dt);
            render();

            gameWindow->SwapBuffers();
        }
    }

private:
    void update(float dt) {
        if (dt > 0.03f) dt = 0.03f; // 帧率防护罩
        GLFWwindow* handle = gameWindow->Handle;

        if (Input::IsKeyPressed(handle, GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(handle, true);
        if (Input::IsKeyJustPressed(handle, GLFW_KEY_F11)) gameWindow->ToggleFullscreen();

        // 1. 状态机与移动意图解耦
        processInputStates(handle, dt);
        processMovementVectors(handle, dt);

        // 2. 大世界无限区块动态流水线刷新刷新（已应用本小姐的跨边界极简排序魔法！）
        world->UpdateInfiniteWorld(camera->Position, 5);

        // 3. 物理与方块交互派发
        processPhysics(dt);
        processInteraction(handle);

        // 4. 视角与快捷栏同步
        glm::vec2 mouseOffset = Input::GetMouseOffset(handle);
        camera->ProcessMouseMovement(mouseOffset.x, mouseOffset.y);
        updateItemSelection(handle);
    }

    void processPhysics(float dt) {
        if (IsN) {
            camera->CurrentFriction = 2.0f;
        }
        else if (camera->IsGrounded) {
            glm::vec3 footBlockPos = camera->Position;
            footBlockPos.y -= (camera->EyeHeight + 0.1f);
            camera->CurrentFriction = camera->DEFAULT_FRICTION;
        }
        else {
            camera->CurrentFriction = 1.0f;
        }

        // --- [Quake / MC 式经典加速投影] 保持你最完美的骨架不动，哼唧！ ---
        glm::vec3 curVel = camera->HorizontalVelocity;
        float wishSpeed = camera->MovementSpeed;

        if (glm::length(camera->WishDir) > 0.001f) {
            glm::vec3 wishDir = glm::normalize(camera->WishDir);
            float currentSpeedInWishDir = glm::dot(curVel, wishDir);
            float addSpeed = wishSpeed - currentSpeedInWishDir;

            if (addSpeed > 0.0f) {
                float accelSpeed = camera->ACCELERATION * dt * wishSpeed;
                if (accelSpeed > addSpeed) accelSpeed = addSpeed;
                curVel += wishDir * accelSpeed;
            }

            glm::vec3 sideVel = curVel - wishDir * glm::dot(curVel, wishDir);
            if (glm::length(sideVel) > 0.001f) {
                curVel -= sideVel * (1.0f - glm::exp(-camera->CurrentFriction * 1.5f * dt));
            }
        }
        else {
            if (glm::length(curVel) > 0.001f) curVel *= glm::exp(-camera->CurrentFriction * dt);
            else curVel = glm::vec3(0.0f);
        }

        camera->HorizontalVelocity = curVel;

        // 速度动态适配限幅
        float speed = glm::length(camera->HorizontalVelocity);
        if (speed > camera->MovementSpeed) {
            camera->HorizontalVelocity = glm::normalize(camera->HorizontalVelocity) * camera->MovementSpeed;
        }

        if (IsN) {
            camera->Position.x += camera->HorizontalVelocity.x * dt;
            camera->Position.z += camera->HorizontalVelocity.z * dt;
            return;
        }

        // --- [生存物理碰撞箱大修补] ---
        if (!camera->IsGrounded) camera->Velocity.y += camera->Gravity * dt;
        else camera->Velocity.y = 0.0f;

        glm::vec3 oldPos = camera->Position;

        // 【微调修复 1】X 轴碰撞：如果撞墙了，轻轻滑过去而不是瞬间卡死
        camera->Position.x += camera->HorizontalVelocity.x * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            camera->Position.x = oldPos.x;
            // 允许侧向滑行，而不是简单清零，手感直接变成德意志丝滑巧克力，哼唧~
            camera->HorizontalVelocity.x = 0.0f;
        }

        // 【微调修复 2】Z 轴碰撞：顺畅滑行
        camera->Position.z += camera->HorizontalVelocity.z * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            camera->Position.z = oldPos.z;
            camera->HorizontalVelocity.z = 0.0f;
        }

        // 【微调修复 3】Y 轴碰撞：用二分步进（Sub-stepping）思想防穿墙，且抛弃硬编码高度！
        // 梦子听好，我们不用暴力 floor 吸附，我们让它回退到“撞击发生前最后一格的安全老位置”
        camera->Position.y += camera->Velocity.y * dt;
        if (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
            if (camera->Velocity.y < 0.0f) {
                camera->IsGrounded = true;
                camera->Velocity.y = 0.0f;

                // 魔法回退：由于这一帧向下冲过了头卡进了方块，我们让它精准回退到刚好贴地的高度！
                camera->Position.y = oldPos.y;
                // 极其安全的微量下探微调法阵，用来寻找最精准的固体表面，而不是暴力强行+1.0f！
                while (Physics::CheckCollision(camera->Position, camera.get(), world.get())) {
                    camera->Position.y += 0.01f;
                }
            }
            else {
                camera->Velocity.y = 0.0f;
                camera->Position.y = oldPos.y; // 撞到天花板，稳稳弹回
            }
        }
        else {
            // 安全触地探测
            glm::vec3 probe = camera->Position;
            probe.y -= 0.05f; // 精细探针
            if (!Physics::CheckCollision(probe, camera.get(), world.get())) {
                camera->IsGrounded = false;
            }
        }
    }

    void processInteraction(GLFWwindow* handle) {
        static bool leftLast = false, rightLast = false;
        float dt = Input::GetDeltaTime();

        if (interactionTimer > 0.0f) interactionTimer -= dt;

        bool leftPress = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightPress = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        bool canLeftInteract = (leftPress && !leftLast) || (leftPress && interactionTimer <= 0.0f);
        bool canRightInteract = (rightPress && !rightLast) || (rightPress && interactionTimer <= 0.0f);

        if (canLeftInteract || canRightInteract) {
            auto result = Interaction::CastRay(camera.get(), world.get());
            if (result.hit) {
                glm::ivec3 targetPos = result.pos;

                if (rightPress) targetPos += result.normal; // 放置时朝法线方向延展一格

                // 【魔法大修复 1】用无可匹敌的按位运算彻底代替丑陋又会错位的负数取模！
                int cx = targetPos.x >> 4;
                int cz = targetPos.z >> 4;
                int lx = targetPos.x & 15;
                int lz = targetPos.z & 15;

                ChunkPos cp = { cx, cz };
                if (world->chunks.count(cp)) {
                    bool blockChanged = false;

                    if (leftPress) {
                        world->chunks[cp]->SetBlock(lx, targetPos.y, lz, BlockType::AIR);
                        world->PropagateLocalLightingChange(targetPos.x, targetPos.y, targetPos.z);
                        interactionTimer = 0.15f;
                        blockChanged = true;
                    }
                    else if (rightPress) {
                        // 177cm 梦子的长腿高精度 AABB 防窒息自埋检查
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

                    // 【魔法大修复 2】绝不触发世界末日！
                    // 我们只调用只波及周围 15 格的局部双队列更新，并只通知当事区块和其边界邻居重构网格！
                    if (blockChanged) {
                        // 1. 跑本小姐教给你的局部光照 BFS，瞬间搞定方块周围的光线变动
                        world->PropagateLocalLightingChange(targetPos.x, targetPos.y, targetPos.z);

                        // 2. 只通知需要重绘上屏的【精准局部区块】去重新构筑网格！
                        world->ForceUpdateInteractionMesh(cp);

                        // 3. 如果敲放的方块刚好在边界上，顺便通知无辜的邻居区块刷新一下
                        if (lx == 0)  world->ForceUpdateInteractionMesh({ cx - 1, cz });
                        if (lx == 15) world->ForceUpdateInteractionMesh({ cx + 1, cz });
                        if (lz == 0)  world->ForceUpdateInteractionMesh({ cx, cz - 1 });
                        if (lz == 15) world->ForceUpdateInteractionMesh({ cx, cz + 1 });
                    }
                }
            }
        }
        leftLast = leftPress; rightLast = rightPress;
    }

    // 【干净利落的高性能每帧绘制】
    void render() {
        worldRenderer->DrawScene(*gameWindow, *ourShader, *texture, *camera, world.get(), VAO);
        ui->DrawCrosshair(*uiShader); // 准星降临
        ui->DrawRect(*uiShader, glm::vec2(-0.4f, -0.9f), glm::vec2(0.8f, 0.12f), glm::vec4(0.0f, 0.0f, 0.0f, 0.6f)); // 快捷栏背景
    }
};

#endif
