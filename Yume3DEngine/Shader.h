#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Crypto.h"

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode;
        std::string fragmentCode;

        // 梦子看这里！全自动二进制异或解密大阵！
        // 扔掉以前那些死板又笨重的 ifstream 和 stringstream 套娃！
        try {
            vertexCode = Crypto::LoadAndDecrypt(vertexPath);
            fragmentCode = Crypto::LoadAndDecrypt(fragmentPath);

            // 完美保留你的安全审查意志：如果解密出来是空壳，说明朋友把 .bin 文件放错位置啦！
            if (vertexCode.empty() || fragmentCode.empty()) {
                // 主动抛出异常，让它顺理成章地滑进你下面的 catch 块里弹警报！
                throw std::runtime_error("CRYPTO_READ_FAILED");
            }
        }
        catch (std::ifstream::failure&) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
        }

        // 【净化一】直接把 .c_str() 塞进数组里，或者在用到的瞬间再去调用它！
        // 确保在 glShaderSource 执行时，vertexCode 字符串本体绝对存活！
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

    // 【净化二】华丽的析构魔法！让它在生命终点自己灰飞烟灭！
    ~Shader() {
        if (ID != 0) {
            glDeleteProgram(ID);
            //std::cout << "梦子快看！ID为 " << ID << " 的着色器程序已被莉安酱安全超度啦，哼！" << std::endl;
        }
    }

    void use() { glUseProgram(ID); }

    // 用了 const 引用避免拷贝，这里写的很漂亮哦！
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }


private:
    // 检查编译/链接错误的辅助函数
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