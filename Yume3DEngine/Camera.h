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
    glm::vec3 HorizontalVelocity = glm::vec3(0.0f);
    bool IsGrounded = false;

    const float PlayerHeight = 1.8f;
    float EyeHeight = 1.62f;
    const float PlayerRadius = 0.3f;
    float Gravity = -20.0f;
    const float JumpForce = 7.0f;
    const float ACCELERATION = 9.0f; 
    float CurrentFriction = 0.0f;         // 改为变量：由物理引擎每帧动态写入
    const float DEFAULT_FRICTION = 15.0f;
    glm::vec3 WishDir = glm::vec3(0.0f);

    float Yaw;
    float Pitch;
    float MovementSpeed = 3.5f;
    float Zoom = 75.0f;

    //  高贵的可配置属性：把鼠标灵敏度提取出来，再也不用硬编码 0.1f 啦！
    float MouseSensitivity = 0.1f;

    //  现代 C++ 规范：把构造函数的所有基础变量在初始化列表里全部排好队，干净利落！
    Camera(glm::vec3 position)
        : Position(position),
        Front(glm::vec3(0.0f, 0.0f, -1.0f)),
        Right(glm::vec3(1.0f, 0.0f, 0.0f)),
        Up(glm::vec3(0.0f, 1.0f, 0.0f)),
        WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
        Yaw(-90.0f), // 现代 OpenGL 默认看向 -Z 轴，设置 -90 度能让开局视线绝对清爽！
        Pitch(0.0f)
    {
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() { return glm::lookAt(Position, Position + Front, Up); }

    // 处理键盘：收集移动方向
    void ProcessKeyboard(int direction) {
        glm::vec3 moveFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
        glm::vec3 moveRight = glm::normalize(glm::vec3(Right.x, 0.0f, Right.z));

        if (direction == 1) WishDir += moveFront;
        if (direction == 2) WishDir -= moveFront;
        if (direction == 3) WishDir -= moveRight;
        if (direction == 4) WishDir += moveRight;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        // 使用高贵的可调灵敏度！
        Yaw += xoffset * MouseSensitivity;
        Pitch += yoffset * MouseSensitivity;

        if (Pitch > 89.0f)  Pitch = 89.0f;
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
