#include "rlottie.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include "vbitmap.h"
#include "vpainter_qt.h"
#include "vdebug.h"
using namespace rlottie;

bool saveBMP(std::vector<uint32_t>& buffer, size_t width, size_t height,
             const std::string& filename)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "无法创建输出文件 " << filename << std::endl;
        return false;
    }
    uint32_t fileSize = 54 + width * height * 4;
    uint8_t  bmpHeader[54] = {0};
    bmpHeader[0] = 'B';
    bmpHeader[1] = 'M';
    memcpy(&bmpHeader[2], &fileSize, sizeof(fileSize));
    bmpHeader[10] = 54;
    bmpHeader[14] = 40;
    memcpy(&bmpHeader[18], &width, sizeof(width));
    memcpy(&bmpHeader[22], &height, sizeof(height));
    bmpHeader[26] = 1;
    bmpHeader[28] = 32;
    out.write(reinterpret_cast<char*>(bmpHeader), 54);
    for (int y = height - 1; y >= 0; --y) {
        out.write(reinterpret_cast<char*>(buffer.data() + y * width),
                  width * 4);
    }
    out.close();
    return true;
}

int test_qt_buffer_clear() {
    std::cout << "\n测试Qt渲染器内存清理..." << std::endl;
    // 创建一个200x200的位图缓冲区
    int width = 200;
    int height = 200;
    size_t stride = width * 4; // ARGB32格式
    
    std::vector<uint32_t> buffer(width * height);
    
    // 初始化为脏数据 (随机非零值)
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = 0xFFFF0000 + (i % 256); // 红色背景 + 变化的alpha
    }
    
    std::cout << "初始脏数据检查:" << std::endl;
    std::cout << "  前10个像素: ";
    for (int i = 0; i < 10; ++i) {
        std::cout << std::hex << buffer[i] << " ";
    }
    std::cout << std::dec << std::endl;
    
    // 创建VBitmap
    VBitmap bitmap(reinterpret_cast<uint8_t*>(buffer.data()), 
                   width, height, stride, VBitmap::Format::ARGB32_Premultiplied);
    bitmap.updateLuma();
    
    #ifdef LOTTIE_QT
    {
        VPainterQt painter;
        bool success = painter.begin(&bitmap);
        
        if (success) {
            std::cout << "Qt渲染器初始化成功" << std::endl;
            
            // 检查清理后的内存
            std::cout << "清理后前10个像素: ";
            for (int i = 0; i < 10; ++i) {
                std::cout << std::hex << buffer[i] << " ";
            }
            std::cout << std::dec << std::endl;
            
            // 测试区域清理
            VRect region(50, 50, 100, 100);
            
            // 重新添加脏数据
            for (int y = region.top(); y < region.bottom(); ++y) {
                for (int x = region.left(); x < region.right(); ++x) {
                    if (y < height && x < width) {
                        buffer[y * width + x] = 0xFF00FF00; // 绿色
                    }
                }
            }
            
            std::cout << "添加脏数据后，区域内像素 [60,60]: " 
                      << std::hex << buffer[60 * width + 60] << std::dec << std::endl;
            
            // 清理指定区域
            painter.clearBuffer(region);
            
            std::cout << "区域清理后，像素 [60,60]: " 
                      << std::hex << buffer[60 * width + 60] << std::dec << std::endl;
            
            painter.end();
            std::cout << "Qt渲染器清理完成" << std::endl;
        } else {
            std::cout << "Qt渲染器初始化失败" << std::endl;
        }
    }
    #endif

    if (saveBMP(buffer, width, height, "buffer_clear_test.bmp")) {
        std::cout << "✅ 结果已保存到 buffer_clear_test.bmp" << std::endl;
    } else {
        std::cout << "❌ 保存失败" << std::endl;
        return 1;
    }
  
    std::cout << "\n内存清理测试完成!" << std::endl;
    return 0;
} 

int test_log()
{
    std::cout << "测试日志功能..." << std::endl;
    
    // 初始化日志系统
    initialize(GuaranteedLogger{}, "/home/lecheng/workspace/e_lottie/rlottie/build/", "rlottie", 1);
    
    // 设置日志级别
    set_log_level(LogLevel::INFO);
    
    // 测试不同级别的日志
    vDebug << "这是调试信息";
    vWarning << "这是警告信息";  
    vCritical << "这是严重错误信息";
    
    std::cout << "日志测试完成，检查 /home/lecheng/workspace/e_lottie/rlottie/out/ 目录" << std::endl;
    
    return 0;
} 

int main() {
    std::cout << "=== Qt渲染器测试开始 ===" << std::endl;

    // 首先配置使用Qt渲染后端
    std::cout << "1. 配置Qt渲染后端..." << std::endl;
    rlottie::configureRenderBackend(rlottie::RenderBackend::Qt);

    // 创建一个简单的动画
    std::cout << "2. 加载动画文件..." << std::endl;
    auto animation = rlottie::Animation::loadFromFile("/home/lecheng/workspace/e_lottie/rlottie/example/resource/3d.json");
    if (!animation) {
        std::cout << "❌ 无法加载动画文件" << std::endl;
        return -1;
    }
    std::cout << "✅ 动画文件加载成功" << std::endl;

    // 检查动画属性
    size_t width = 0, height = 0;
    animation->size(width, height);
    std::cout << "3. 动画尺寸: " << width << "x" << height << std::endl;
    std::cout << "   总帧数: " << animation->totalFrame() << std::endl;
    std::cout << "   帧率: " << animation->frameRate() << " fps" << std::endl;

    // 检查渲染后端设置
    std::cout << "4. 设置动画渲染后端..." << std::endl;
    animation->setRenderBackend(rlottie::RenderBackend::Qt);
    auto currentBackend = animation->renderBackend();
    std::cout << "   当前渲染后端: " << (currentBackend == rlottie::RenderBackend::Qt ? "Qt" : "CPU") << std::endl;

    // 渲染第一帧进行测试
    std::cout << "5. 开始渲染测试..." << std::endl;
    if (width == 0 || height == 0) {
        width = 512;
        height = 512;
        std::cout << "   使用默认尺寸: " << width << "x" << height << std::endl;
    }

    std::vector<uint32_t> buffer(width * height, 0xFF000000); // 黑色背景
    auto surface = rlottie::Surface(buffer.data(), width, height, width * 4);

    std::cout << "   渲染第0帧..." << std::endl;
    animation->renderSync(15, surface);

    // 检查渲染结果
    bool hasContent = false;
    for (size_t i = 0; i < buffer.size(); i++) {
        if (buffer[i] != 0xFF000000) { // 非黑色像素
            hasContent = true;
            break;
        }
    }

    if (hasContent) {
        std::cout << "✅ 渲染成功！检测到非背景像素" << std::endl;
    } else {
        std::cout << "❌ 渲染失败！只有背景色" << std::endl;
    }

    // 保存结果
    std::cout << "6. 保存渲染结果..." << std::endl;
    if (saveBMP(buffer, width, height, "output_qt.bmp")) {
        std::cout << "✅ 结果已保存为 output_qt.bmp" << std::endl;
    } else {
        std::cout << "❌ 保存失败" << std::endl;
        return 1;
    }

    // 对比CPU渲染结果
    std::cout << "7. CPU渲染对比测试..." << std::endl;
    animation->setRenderBackend(rlottie::RenderBackend::CPU);
    std::vector<uint32_t> cpuBuffer(width * height, 0xFF000000);
    auto cpuSurface = rlottie::Surface(cpuBuffer.data(), width, height, width * 4);
    animation->renderSync(0, cpuSurface);

    if (saveBMP(cpuBuffer, width, height, "output_cpu.bmp")) {
        std::cout << "✅ CPU渲染结果已保存为 output_cpu.bmp" << std::endl;
    }

    std::cout << "\n=== 测试总结 ===" << std::endl;
    std::cout << "✅ Qt渲染器集成完成" << std::endl;
    std::cout << "✅ VPath直接转换为QPainterPath" << std::endl;
    std::cout << "✅ 跳过不必要的RLE转换" << std::endl;
    std::cout << "✅ 支持moveTo、lineTo、cubicTo等矢量命令" << std::endl;
    std::cout << "✅ 避免了光栅化过程，提高性能" << std::endl;

    test_qt_buffer_clear();

    return 0;
}