#include "rlottie.h"
#include "vdebug.h"
#include <iostream>
#include <thread>

int main()
{
    std::cout << "测试日志功能..." << std::endl;
    
    // 初始化日志系统
    initialize(GuaranteedLogger{}, "/home/lecheng/workspace/e_lottie/rlottie/out/", "rlottie", 1);
    
    // 设置日志级别
    set_log_level(LogLevel::INFO);
    
    // 测试不同级别的日志
    vDebug << "这是调试信息";
    vWarning << "这是警告信息";  
    vCritical << "这是严重错误信息";
    
    std::cout << "日志测试完成，检查 /home/lecheng/workspace/e_lottie/rlottie/out/ 目录" << std::endl;
    
    // 等待一点时间让日志写入
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return 0;
} 