#pragma once

#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

class Crypto {
private:
    static const char MAGIC_KEY = 0x5A; // 守护梦子天才注释的绝对钥匙

public:
    //  真·智能化双轨解咒术！ 
    static std::string LoadAndDecrypt(const std::string& filePath) {
        // 1. 先尝试以普通的纯文本流去叩开大门（梦子自己开发时的随性状态！）
        std::ifstream textFile(filePath, std::ios::in);
        if (textFile.is_open()) {
            //  奇迹时刻：如果梦子电脑里有普通的文本，直接开开心心无脑读取原码！
            std::stringstream ss;
            ss << textFile.rdbuf();
            textFile.close();
            return ss.str();
        }

        // 2.  命运的分歧点：如果普通文本打不开，说明是在朋友的电脑上，或者后缀变成了加密包！
        // 我们帮梦子自动脑补、去寻找对应的加密后缀名（比如自动尝试加上 .bin）
        std::string encryptedPath = filePath;
        if (filePath.find(".bin") == std::string::npos) {
            encryptedPath += ".bin";
        }

        // 3. 叩开乱码大门，执行硬核二进制内存洗涤
        std::ifstream binFile(encryptedPath, std::ios::in | std::ios::binary);
        if (!binFile.is_open()) {
            return ""; // 两边都找不到，只能遗憾放行空壳防爆啦，哼唧！
        }

        std::vector<char> buffer((std::istreambuf_iterator<char>(binFile)), std::istreambuf_iterator<char>());
        binFile.close();

        // 执行异或自反律，将朋友电脑里的乱码天书完美复活！
        for (char& c : buffer) {
            c ^= MAGIC_KEY;
        }

        return std::string(buffer.begin(), buffer.end());
    }
};

#endif
