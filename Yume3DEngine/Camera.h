#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // --- 空间属性 ---
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Right;
    glm::vec3 Up;
    glm::vec3 WorldUp;

    // --- 物理属性 (梦子 177cm 专用数据) ---
    glm::vec3 Velocity = glm::vec3(0.0f);
    glm::vec3 HorizontalVelocity = glm::vec3(0.0f); // 这一帧我想走的方向
    bool IsGrounded = false;

    const float PlayerHeight = 1.8f;  // 全身高
    float EyeHeight = 1.62f;     // 眼睛距离脚底的高度
    const float PlayerRadius = 0.3f;   // 胖度（碰撞半径）
    float Gravity = -20.0f;      // 重力加速度
    const float JumpForce = 7.0f;      // 跳跃初速度

    float Yaw;
    float Pitch;
    float MovementSpeed = 5.0f; // 9km/h 快走
    float Zoom = 60.0f;

    Camera(glm::vec3 position) : Position(position), WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)), Yaw(0.0f), Pitch(0.0f) {
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() { return glm::lookAt(Position, Position + Front, Up); }

    // 处理键盘：现在只负责收集“我想往哪走”的意愿
    void ProcessKeyboard(int direction, float dt) {
        glm::vec3 moveFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
        glm::vec3 moveRight = glm::normalize(glm::vec3(Right.x, 0.0f, Right.z));

        // 只做纯粹的方向向量叠加，不在这里执行归一化和乘速度，哼唧！
        if (direction == 1) HorizontalVelocity += moveFront;
        if (direction == 2) HorizontalVelocity -= moveFront;
        if (direction == 3) HorizontalVelocity -= moveRight;
        if (direction == 4) HorizontalVelocity += moveRight;
    }


    void ProcessMouseMovement(float xoffset, float yoffset) {
        Yaw += xoffset * 0.1f;
        Pitch += yoffset * 0.1f;
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};