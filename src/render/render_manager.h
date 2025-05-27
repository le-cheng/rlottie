/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include "rlottie.h"
#include "vpainter.h"
#include <string>
#include <vector>
#include <memory>

V_BEGIN_NAMESPACE

/**
 * @brief 渲染器能力信息
 */
struct RendererCapability {
    rlottie::RenderBackend backend;
    std::string name;
    std::string description;
    bool available;
    bool hardwareAccelerated;
    bool supportsGradients;
    bool supportsMasks;
    bool supportsFilters;
    int priority; // 优先级，数值越大优先级越高
};

/**
 * @brief 渲染器性能信息
 */
struct RendererPerformance {
    double averageFPS;
    double peakFPS;
    size_t memoryUsage; // 内存使用量(字节)
    double gpuUtilization; // GPU使用率(0-100)
    size_t renderedFrames;
    double totalRenderTime; // 总渲染时间(毫秒)
};

/**
 * @brief 渲染器管理器
 * 
 * 提供统一的渲染后端管理接口，包括：
 * - 自动检测可用的渲染后端
 * - 智能选择最佳渲染器
 * - 运行时切换渲染器
 * - 渲染器性能监控
 * - 降级回退机制
 */
class RenderManager {
public:
    static RenderManager& instance();
    
    // 初始化渲染管理器
    bool initialize();
    
    // 关闭渲染管理器
    void shutdown();
    
    // 检测所有可用的渲染器
    void detectAvailableRenderers();
    
    // 获取所有渲染器能力信息
    const std::vector<RendererCapability>& getCapabilities() const;
    
    // 获取指定渲染器的能力信息
    RendererCapability getCapability(rlottie::RenderBackend backend) const;
    
    // 检查渲染器是否可用
    bool isRendererAvailable(rlottie::RenderBackend backend) const;
    
    // 自动选择最佳渲染器
    rlottie::RenderBackend selectBestRenderer() const;
    
    // 为特定用途选择渲染器
    rlottie::RenderBackend selectRendererForPurpose(
        bool needsHardwareAccel = false,
        bool needsGradients = false,
        bool needsMasks = false
    ) const;
    
    // 创建渲染器实例
    std::unique_ptr<VPainter> createRenderer(rlottie::RenderBackend backend) const;
    
    // 设置默认渲染器
    void setDefaultRenderer(rlottie::RenderBackend backend);
    
    // 获取默认渲染器
    rlottie::RenderBackend getDefaultRenderer() const;
    
    // 渲染器性能监控
    void startPerformanceMonitoring(rlottie::RenderBackend backend);
    void stopPerformanceMonitoring(rlottie::RenderBackend backend);
    RendererPerformance getPerformanceStats(rlottie::RenderBackend backend) const;
    void resetPerformanceStats(rlottie::RenderBackend backend);
    
    // 渲染器热切换
    bool switchRenderer(rlottie::RenderBackend from, rlottie::RenderBackend to);
    
    // 降级回退机制
    rlottie::RenderBackend getFallbackRenderer(rlottie::RenderBackend failed) const;
    
    // 调试和诊断
    void printCapabilities() const;
    void printPerformanceReport() const;
    std::string getDiagnosticInfo() const;
    
    // 配置选项
    void setEnableAutoFallback(bool enable) { mAutoFallback = enable; }
    void setPerformanceMonitoring(bool enable) { mPerformanceMonitoring = enable; }
    void setVerboseLogging(bool enable) { mVerboseLogging = enable; }

private:
    RenderManager() = default;
    ~RenderManager() = default;
    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;
    
    // 检测特定渲染器
    bool detectCPURenderer();
    bool detectQtRenderer();
    bool detectVGLiteRenderer();
    bool detectOpenGLRenderer();
    bool detectVulkanRenderer();
    
    // 内部辅助方法
    void addCapability(const RendererCapability& cap);
    RendererCapability createCapability(
        rlottie::RenderBackend backend,
        const std::string& name,
        const std::string& description,
        bool available,
        bool hardwareAccel = false,
        int priority = 0
    );

private:
    std::vector<RendererCapability> mCapabilities;
    std::vector<RendererPerformance> mPerformanceStats;
    rlottie::RenderBackend mDefaultRenderer = rlottie::RenderBackend::CPU;
    bool mInitialized = false;
    bool mAutoFallback = true;
    bool mPerformanceMonitoring = false;
    bool mVerboseLogging = false;
};

/**
 * @brief 渲染器工厂类
 * 
 * 简化的工厂接口，内部使用RenderManager
 */
class RendererFactory {
public:
    // 创建默认渲染器
    static std::unique_ptr<VPainter> createDefault();
    
    // 创建指定类型的渲染器
    static std::unique_ptr<VPainter> create(rlottie::RenderBackend backend);
    
    // 创建最佳渲染器
    static std::unique_ptr<VPainter> createBest();
    
    // 检查渲染器是否可用
    static bool isAvailable(rlottie::RenderBackend backend);
    
    // 获取推荐的渲染器
    static rlottie::RenderBackend getRecommended();
    
    // 列出所有可用的渲染器
    static std::vector<rlottie::RenderBackend> listAvailable();
};

V_END_NAMESPACE

#endif // RENDER_MANAGER_H 