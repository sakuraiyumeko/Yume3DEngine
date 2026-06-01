#pragma once

#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>
#include <iostream>

//  核心大净化：把原本在头文件里的 #define STB_IMAGE_IMPLEMENTATION 丢进历史垃圾桶！
// 梦子注意：这个宏只能、且必须写在全项目【唯一一个】普通的 .cpp 文件最顶端噢！
#include "stb_image.h" 

class Texture {
public:
    unsigned int ID;

    Texture(const char* imagePath, GLenum format = GL_RGB) : ID(0) {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        // 为梦子默认设置像素风最爱的 NEAREST 过滤
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        int width, height, nrChannels;

        // 梦子注意：stbi 全局状态翻转在多线程时会有小干扰，但目前的架构它能完美让贴图不再颠倒
        stbi_set_flip_vertically_on_load(true);

        unsigned char* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else {
            std::cout << "Failed to load texture at: " << imagePath << std::endl;
        }
        stbi_image_free(data);

        //  规范化防污染：加载完纹理之后，随手把它解绑（glBindTexture 0），
        // 这样可以绝对避免以后加载别的 2D UI 纹理时发生诡异的数据窜位，哼唧！
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // 完美的常数槽位激活渲染接口
    void use(unsigned int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, ID);
    }

    ~Texture() {
        if (ID != 0) {
            glDeleteTextures(1, &ID);
        }
    }
};

#endif
