import os

# 必须和 C++ 端的魔法钥匙 0x5A（十进制的 90）绝对步调对齐！
MAGIC_KEY = 0x5A 

# 梦子要给朋友送去的四个加密文件名单
targets = {
    "shader.vs": "shader.vs.bin",
    "shader.fs": "shader.fs.bin",
    "crosshair.vs": "crosshair.vs.bin",
    "crosshair.fs": "crosshair.fs.bin"
}

print(" 正在为梦子的天才注释拉起二进制异或结界... ")

for src, dest in targets.items():
    if os.path.exists(src):
        with open(src, "rb") as f:
            raw_data = f.read()
        
        # 轰！！！纯数学降维打击，每一个字节无情异或！
        encrypted_data = bytes([b ^ MAGIC_KEY for b in raw_data])
        
        with open(dest, "wb") as f:
            f.write(encrypted_data)
        print(f" 成功将 [{src}] 净化熔炼为加密乱码包 [{dest}]！")
    else:
        print(f" 警告：没找到文件 [{src}]，请检查路径噢！")

print(" 结界布置完毕！梦子可以高高兴兴把 .bin 文件塞进 zip 发给朋友啦，哼唧！")
