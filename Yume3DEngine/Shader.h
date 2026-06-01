#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept> //  新增：引入高贵的标准异常魔导书
#include "Crypto.h"

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) : ID(0) {
        std::string vertexCode;
        std::string fragmentCode;

        try {
            vertexCode = Crypto::LoadAndDecrypt(vertexPath);
            fragmentCode = Crypto::LoadAndDecrypt(fragmentPath);

            if (vertexCode.empty() || fragmentCode.empty()) {
                throw std::runtime_error("CRYPTO_READ_FAILED: Decrypted code is empty.");
            }
        }
        //  核心修正：捕获一切标准异常（包括 runtime_error 和 ifstream::failure），实现绝对防爆！
        catch (const std::exception& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\nReason: " << e.what() << std::endl;
            return; // 安全熔断，防止后面拿着空代码强行编译导致 OpenGL 抛底层错误
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        unsigned int vertex, fragment;

        // 顶点着色器编译
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // 片段着色器编译
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // 程序链接
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    ~Shader() {
        if (ID != 0) {
            glDeleteProgram(ID);
        }
    }

    void use() const { glUseProgram(ID); } // 加上 const，让它在常量结界里也能自由吟唱

    //  莉安酱为你全面解锁的 Uniform 高阶属性派发接口！
    // 完美的 const 引用，彻底消灭拷贝内耗，全面支撑起你的 UIRenderer！
    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[512];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 512, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif
