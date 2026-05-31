#pragma once

#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string>
#include <iostream>

class Texture {
public:
    unsigned int ID;

    Texture(const char* imagePath, GLenum format = GL_RGB) {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        // 为梦子默认设置像素风最爱的 NEAREST 过滤
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true); // 让贴图不再颠倒
        unsigned char* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else {
            std::cout << "Failed to load texture at: " << imagePath << std::endl;
        }
        stbi_image_free(data);
    }

    void use(unsigned int unit = 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, ID);
    }
    ~Texture() {
        if (ID != 0) {
            glDeleteTextures(1, &ID);
            //std::cout << "梦子快看，ID为 " << ID << " 的纹理已经被莉安酱完美回收啦！" << std::endl;
        }
    }
};
#endif