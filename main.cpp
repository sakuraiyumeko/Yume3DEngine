#define STB_IMAGE_IMPLEMENTATION
#include "Yume3DEngine/Game.h" // 引入我们唯一的宇宙总装核心！


int main() {
    // 1. 召唤属于梦子的极致自研体素游戏引擎实体！
    Game voxelEngine;

    // 2. 唤醒生命周期一：奠基环境，在内存里自动筑起 3 层 3D 实例化草地大平原
    voxelEngine.Init(1240, 720, "Sakurai Yumeko's Supreme Engine");

    // 3. 唤醒生命周期二：时空巨轮启动！每一帧的物理、时钟和画面，全都在内部狂暴运转！
    voxelEngine.RunLoop();

    // 4. 优雅退场，所有的显卡垃圾和内存泄漏都会被析构函数自动超度！
    return 0;
}
