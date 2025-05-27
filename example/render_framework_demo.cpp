/*
 * 渲染框架演示程序
 * 展示多渲染后端的灵活切换和管理
 */

#include <iostream>
#include <chrono>
#include <memory>
#include <string>

#include "rlottie.h"

#if defined(__has_include)
#  if __has_include("config.h")
#    include "config.h"
#  endif
#endif

// 如果有渲染管理器，可以使用
#ifdef LOTTIE_RENDER_MANAGER
#include "render_manager.h"
#endif

class RenderDemo {
public:
    RenderDemo() = default;

    void run() {
        std::cout << "=== rlottie 渲染框架演示 ===" << std::endl;
        
        // 1. 检测可用的渲染器
        detectRenderers();
        
        // 2. 测试不同渲染器的性能
        testRendererPerformance();
        
        // 3. 演示智能渲染器选择
        demoSmartSelection();
        
        // 4. 演示运行时切换
        demoRuntimeSwitch();
        
        std::cout << "\n=== 演示完成 ===" << std::endl;
    }

private:
    void detectRenderers() {
        std::cout << "\n1. 检测可用的渲染器..." << std::endl;

#ifdef LOTTIE_RENDER_MANAGER
        auto& manager = RenderManager::instance();
        manager.initialize();
        manager.setVerboseLogging(true);
        
        std::cout << "\n使用高级渲染管理器：" << std::endl;
        manager.printCapabilities();
#else
        std::cout << "\n使用基础渲染检测：" << std::endl;
        
        // 测试CPU渲染器
        testRenderer(rlottie::RenderBackend::CPU, "CPU渲染器");
        
        // 测试Qt渲染器
        testRenderer(rlottie::RenderBackend::Qt, "Qt渲染器");
        
        // 测试VGLite渲染器
        testRenderer(rlottie::RenderBackend::VGLite, "VGLite渲染器");
#endif
    }

    void testRenderer(rlottie::RenderBackend backend, const std::string& name) {
        std::cout << "测试 " << name << ": ";
        
        try {
            rlottie::configureRenderBackend(backend);
            auto animation = rlottie::Animation::loadFromFile(
                std::string(DEMO_DIR) + "test.json"
            );
            
            if (animation) {
                animation->setRenderBackend(backend);
                auto currentBackend = animation->renderBackend();
                
                if (currentBackend == backend) {
                    std::cout << "✓ 可用" << std::endl;
                } else {
                    std::cout << "✗ 回退到其他渲染器" << std::endl;
                }
            } else {
                std::cout << "✗ 动画加载失败" << std::endl;
            }
        } catch (...) {
            std::cout << "✗ 异常" << std::endl;
        }
    }

    void testRendererPerformance() {
        std::cout << "\n2. 渲染器性能测试..." << std::endl;
        
        std::string filePath = std::string(DEMO_DIR) + "test.json";
        
        std::vector<rlottie::RenderBackend> backends = {
            rlottie::RenderBackend::CPU,
            rlottie::RenderBackend::Qt,
            rlottie::RenderBackend::VGLite
        };
        
        std::vector<std::string> names = {
            "CPU", "Qt", "VGLite"
        };
        
        for (size_t i = 0; i < backends.size(); ++i) {
            performanceTest(backends[i], names[i], filePath);
        }
    }

    void performanceTest(rlottie::RenderBackend backend, 
                        const std::string& name, 
                        const std::string& filePath) {
        std::cout << "\n测试 " << name << " 渲染器性能：" << std::endl;
        
        auto animation = rlottie::Animation::loadFromFile(filePath);
        if (!animation) {
            std::cout << "  ❌ 动画加载失败" << std::endl;
            return;
        }
        
        animation->setRenderBackend(backend);
        auto actualBackend = animation->renderBackend();
        
        if (actualBackend != backend) {
            std::cout << "  ⚠️  回退到其他渲染器" << std::endl;
        }
        
        size_t width = 0, height = 0;
        animation->size(width, height);
        
        // 创建渲染缓冲区
        std::vector<uint32_t> buffer(width * height);
        rlottie::Surface surface(buffer.data(), width, height, width * 4);
        
        // 性能测试
        const int testFrames = 10;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < testFrames; ++frame) {
            animation->renderSync(frame % animation->totalFrame(), surface);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(endTime - startTime).count();
        double fps = testFrames / duration;
        
        std::cout << "  渲染 " << testFrames << " 帧用时: " << duration << " 秒" << std::endl;
        std::cout << "  平均FPS: " << fps << std::endl;
        std::cout << "  动画尺寸: " << width << "x" << height << std::endl;
        std::cout << "  总帧数: " << animation->totalFrame() << std::endl;
    }

    void demoSmartSelection() {
        std::cout << "\n3. 智能渲染器选择演示..." << std::endl;

#ifdef LOTTIE_RENDER_MANAGER
        auto& manager = RenderManager::instance();
        
        // 自动选择最佳渲染器
        auto best = manager.selectBestRenderer();
        std::cout << "自动选择的最佳渲染器: " 
                  << manager.getCapability(best).name << std::endl;
        
        // 针对特定需求选择渲染器
        auto hardwareAccel = manager.selectRendererForPurpose(true, false, false);
        std::cout << "硬件加速渲染器: " 
                  << manager.getCapability(hardwareAccel).name << std::endl;
        
        auto withGradients = manager.selectRendererForPurpose(false, true, true);
        std::cout << "支持渐变和遮罩的渲染器: " 
                  << manager.getCapability(withGradients).name << std::endl;
#else
        std::cout << "智能选择需要编译渲染管理器支持" << std::endl;
        
        // 简单的选择逻辑
        auto animation = rlottie::Animation::loadFromFile(
            std::string(DEMO_DIR) + "test.json"
        );
        if (animation) {
            // 尝试VGLite -> Qt -> CPU
            std::vector<rlottie::RenderBackend> preferences = {
                rlottie::RenderBackend::VGLite,
                rlottie::RenderBackend::Qt,
                rlottie::RenderBackend::CPU
            };
            
            for (auto backend : preferences) {
                animation->setRenderBackend(backend);
                if (animation->renderBackend() == backend) {
                    std::cout << "选择渲染器: " << getBackendName(backend) << std::endl;
                    break;
                }
            }
        }
#endif
    }

    void demoRuntimeSwitch() {
        std::cout << "\n4. 运行时渲染器切换演示..." << std::endl;
        
        auto animation = rlottie::Animation::loadFromFile(
            std::string(DEMO_DIR) + "test.json"
        );
        if (!animation) {
            std::cout << "动画加载失败" << std::endl;
            return;
        }
        
        size_t width = 0, height = 0;
        animation->size(width, height);
        std::vector<uint32_t> buffer(width * height);
        rlottie::Surface surface(buffer.data(), width, height, width * 4);
        
        std::vector<rlottie::RenderBackend> backends = {
            rlottie::RenderBackend::CPU,
            rlottie::RenderBackend::Qt,
            rlottie::RenderBackend::VGLite
        };
        
        for (auto backend : backends) {
            std::cout << "\n切换到 " << getBackendName(backend) << " 渲染器..." << std::endl;
            
            animation->setRenderBackend(backend);
            auto actual = animation->renderBackend();
            
            if (actual == backend) {
                std::cout << "  ✓ 切换成功" << std::endl;
            } else {
                std::cout << "  ⚠️  回退到 " << getBackendName(actual) << std::endl;
            }
            
            // 渲染一帧作为测试
            auto start = std::chrono::high_resolution_clock::now();
            animation->renderSync(0, surface);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << "  渲染一帧用时: " << duration << " 毫秒" << std::endl;
        }
    }

    std::string getBackendName(rlottie::RenderBackend backend) {
        switch (backend) {
        case rlottie::RenderBackend::CPU:
            return "CPU";
        case rlottie::RenderBackend::Qt:
            return "Qt";
        case rlottie::RenderBackend::VGLite:
            return "VGLite";
        case rlottie::RenderBackend::OpenGL:
            return "OpenGL";
        case rlottie::RenderBackend::Vulkan:
            return "Vulkan";
        case rlottie::RenderBackend::Custom:
            return "Custom";
        default:
            return "Unknown";
        }
    }
};

int main(int argc, char* argv[])
{
    std::cout << "rlottie 版本信息:" << std::endl;
    std::cout << "  构建时间: " << __DATE__ << " " << __TIME__ << std::endl;
    
#ifdef LOTTIE_QT
    std::cout << "  Qt 支持: 启用" << std::endl;
#else
    std::cout << "  Qt 支持: 禁用" << std::endl;
#endif

#ifdef LOTTIE_VGLITE
    std::cout << "  VGLite 支持: 启用" << std::endl;
#else
    std::cout << "  VGLite 支持: 禁用" << std::endl;
#endif

#ifdef LOTTIE_RENDER_MANAGER
    std::cout << "  渲染管理器: 启用" << std::endl;
#else
    std::cout << "  渲染管理器: 禁用" << std::endl;
#endif

    RenderDemo demo;
    demo.run();

    return 0;
} 