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

#include "render_manager.h"
#include "vdebug.h"
#include <algorithm>
#include <iostream>
#include <sstream>

V_BEGIN_NAMESPACE

RenderManager& RenderManager::instance()
{
    static RenderManager instance;
    return instance;
}

bool RenderManager::initialize()
{
    if (mInitialized) return true;
    
    vDebug << "初始化渲染器管理器...";
    
    // 清空之前的状态
    mCapabilities.clear();
    mPerformanceStats.clear();
    
    // 检测所有可用的渲染器
    detectAvailableRenderers();
    
    // 选择默认渲染器
    mDefaultRenderer = selectBestRenderer();
    
    mInitialized = true;
    
    if (mVerboseLogging) {
        printCapabilities();
    }
    
    vDebug << "渲染器管理器初始化完成，默认渲染器：" 
           << getCapability(mDefaultRenderer).name;
    
    return true;
}

void RenderManager::shutdown()
{
    if (!mInitialized) return;
    
    vDebug << "关闭渲染器管理器...";
    
    mCapabilities.clear();
    mPerformanceStats.clear();
    mInitialized = false;
    
    vDebug << "渲染器管理器已关闭";
}

void RenderManager::detectAvailableRenderers()
{
    vDebug << "开始检测可用的渲染器...";
    
    // 检测CPU渲染器（总是可用）
    detectCPURenderer();
    
    // 检测Qt渲染器
    detectQtRenderer();
    
    // 检测VGLite渲染器
    detectVGLiteRenderer();
    
    // 检测OpenGL渲染器
    detectOpenGLRenderer();
    
    // 检测Vulkan渲染器
    detectVulkanRenderer();
    
    vDebug << "渲染器检测完成，共找到" << mCapabilities.size() << "个渲染器";
}

bool RenderManager::detectCPURenderer()
{
    RendererCapability cap = createCapability(
        rlottie::RenderBackend::CPU,
        "CPU渲染器",
        "软件CPU渲染，兼容性最佳",
        true,  // 总是可用
        false, // 非硬件加速
        10     // 基础优先级
    );
    cap.supportsGradients = true;
    cap.supportsMasks = true;
    cap.supportsFilters = true;
    
    addCapability(cap);
    vDebug << "✓ CPU渲染器可用";
    return true;
}

bool RenderManager::detectQtRenderer()
{
#ifdef LOTTIE_QT
    try {
        // 尝试创建Qt渲染器测试可用性
        auto testRenderer = VPainter::create(RenderType::Qt);
        if (testRenderer && testRenderer->renderType() == RenderType::Qt) {
            RendererCapability cap = createCapability(
                rlottie::RenderBackend::Qt,
                "Qt渲染器",
                "基于Qt QPainter的矢量渲染",
                true,
                true,  // 硬件加速
                50     // 高优先级
            );
            cap.supportsGradients = true;
            cap.supportsMasks = true;
            cap.supportsFilters = false;
            
            addCapability(cap);
            vDebug << "✓ Qt渲染器可用";
            return true;
        }
    } catch (...) {
        vWarning << "Qt渲染器初始化失败";
    }
#endif
    
    vDebug << "✗ Qt渲染器不可用";
    return false;
}

bool RenderManager::detectVGLiteRenderer()
{
#ifdef LOTTIE_VGLITE
    try {
        // 尝试创建VGLite渲染器测试可用性
        auto testRenderer = VPainter::create(RenderType::VGLite);
        if (testRenderer && testRenderer->renderType() == RenderType::VGLite) {
            RendererCapability cap = createCapability(
                rlottie::RenderBackend::VGLite,
                "VGLite渲染器",
                "VGLite 2D GPU硬件加速渲染",
                true,
                true,  // 硬件加速
                80     // 最高优先级
            );
            cap.supportsGradients = true;
            cap.supportsMasks = true;
            cap.supportsFilters = false;
            
            addCapability(cap);
            vDebug << "✓ VGLite渲染器可用";
            return true;
        }
    } catch (...) {
        vWarning << "VGLite渲染器初始化失败";
    }
#endif
    
    vDebug << "✗ VGLite渲染器不可用";
    return false;
}

bool RenderManager::detectOpenGLRenderer()
{
    // TODO: 实现OpenGL渲染器检测
    vDebug << "✗ OpenGL渲染器未实现";
    return false;
}

bool RenderManager::detectVulkanRenderer()
{
    // TODO: 实现Vulkan渲染器检测
    vDebug << "✗ Vulkan渲染器未实现";
    return false;
}

const std::vector<RendererCapability>& RenderManager::getCapabilities() const
{
    return mCapabilities;
}

RendererCapability RenderManager::getCapability(rlottie::RenderBackend backend) const
{
    auto it = std::find_if(mCapabilities.begin(), mCapabilities.end(),
        [backend](const RendererCapability& cap) {
            return cap.backend == backend;
        });
    
    if (it != mCapabilities.end()) {
        return *it;
    }
    
    // 返回一个无效的能力信息
    return RendererCapability{};
}

bool RenderManager::isRendererAvailable(rlottie::RenderBackend backend) const
{
    auto cap = getCapability(backend);
    return cap.available;
}

rlottie::RenderBackend RenderManager::selectBestRenderer() const
{
    if (mCapabilities.empty()) {
        return rlottie::RenderBackend::CPU;
    }
    
    // 按优先级排序，选择可用的最高优先级渲染器
    auto bestIt = std::max_element(mCapabilities.begin(), mCapabilities.end(),
        [](const RendererCapability& a, const RendererCapability& b) {
            if (a.available && b.available) {
                return a.priority < b.priority;
            }
            return !a.available && b.available;
        });
    
    if (bestIt != mCapabilities.end() && bestIt->available) {
        return bestIt->backend;
    }
    
    return rlottie::RenderBackend::CPU;
}

rlottie::RenderBackend RenderManager::selectRendererForPurpose(
    bool needsHardwareAccel,
    bool needsGradients,
    bool needsMasks) const
{
    for (const auto& cap : mCapabilities) {
        if (!cap.available) continue;
        
        bool suitable = true;
        if (needsHardwareAccel && !cap.hardwareAccelerated) suitable = false;
        if (needsGradients && !cap.supportsGradients) suitable = false;
        if (needsMasks && !cap.supportsMasks) suitable = false;
        
        if (suitable) {
            return cap.backend;
        }
    }
    
    // 如果没有完全匹配的，返回最佳可用渲染器
    return selectBestRenderer();
}

std::unique_ptr<VPainter> RenderManager::createRenderer(rlottie::RenderBackend backend) const
{
    if (!isRendererAvailable(backend)) {
        if (mAutoFallback) {
            vWarning << "渲染器" << getCapability(backend).name 
                     << "不可用，回退到默认渲染器";
            backend = mDefaultRenderer;
        } else {
            vCritical << "渲染器" << getCapability(backend).name << "不可用";
            return nullptr;
        }
    }
    
    RenderType renderType;
    switch (backend) {
    case rlottie::RenderBackend::CPU:
        renderType = RenderType::CPU;
        break;
    case rlottie::RenderBackend::Qt:
        renderType = RenderType::Qt;
        break;
    case rlottie::RenderBackend::VGLite:
        renderType = RenderType::VGLite;
        break;
    case rlottie::RenderBackend::OpenGL:
        renderType = RenderType::OpenGL;
        break;
    case rlottie::RenderBackend::Vulkan:
        renderType = RenderType::Vulkan;
        break;
    default:
        renderType = RenderType::CPU;
        break;
    }
    
    return VPainter::create(renderType);
}

void RenderManager::setDefaultRenderer(rlottie::RenderBackend backend)
{
    if (isRendererAvailable(backend)) {
        mDefaultRenderer = backend;
        vDebug << "默认渲染器设置为：" << getCapability(backend).name;
    } else {
        vWarning << "无法设置不可用的渲染器为默认：" << getCapability(backend).name;
    }
}

rlottie::RenderBackend RenderManager::getDefaultRenderer() const
{
    return mDefaultRenderer;
}

rlottie::RenderBackend RenderManager::getFallbackRenderer(rlottie::RenderBackend failed) const
{
    // 简单的回退策略：VGLite -> Qt -> CPU
    switch (failed) {
    case rlottie::RenderBackend::VGLite:
        if (isRendererAvailable(rlottie::RenderBackend::Qt)) {
            return rlottie::RenderBackend::Qt;
        }
        return rlottie::RenderBackend::CPU;
    
    case rlottie::RenderBackend::Qt:
    case rlottie::RenderBackend::OpenGL:
    case rlottie::RenderBackend::Vulkan:
        return rlottie::RenderBackend::CPU;
        
    default:
        return rlottie::RenderBackend::CPU;
    }
}

void RenderManager::printCapabilities() const
{
    std::cout << "\n=== 渲染器能力报告 ===" << std::endl;
    std::cout << "总计：" << mCapabilities.size() << " 个渲染器" << std::endl;
    
    for (const auto& cap : mCapabilities) {
        std::cout << "\n渲染器：" << cap.name << std::endl;
        std::cout << "  描述：" << cap.description << std::endl;
        std::cout << "  状态：" << (cap.available ? "✓ 可用" : "✗ 不可用") << std::endl;
        std::cout << "  硬件加速：" << (cap.hardwareAccelerated ? "是" : "否") << std::endl;
        std::cout << "  优先级：" << cap.priority << std::endl;
        std::cout << "  功能支持：" << std::endl;
        std::cout << "    渐变：" << (cap.supportsGradients ? "支持" : "不支持") << std::endl;
        std::cout << "    遮罩：" << (cap.supportsMasks ? "支持" : "不支持") << std::endl;
        std::cout << "    滤镜：" << (cap.supportsFilters ? "支持" : "不支持") << std::endl;
    }
    
    std::cout << "\n默认渲染器：" << getCapability(mDefaultRenderer).name << std::endl;
    std::cout << "========================\n" << std::endl;
}

std::string RenderManager::getDiagnosticInfo() const
{
    std::ostringstream oss;
    oss << "RenderManager诊断信息:\n";
    oss << "  初始化状态: " << (mInitialized ? "已初始化" : "未初始化") << "\n";
    oss << "  可用渲染器数量: " << mCapabilities.size() << "\n";
    oss << "  默认渲染器: " << getCapability(mDefaultRenderer).name << "\n";
    oss << "  自动回退: " << (mAutoFallback ? "启用" : "禁用") << "\n";
    oss << "  性能监控: " << (mPerformanceMonitoring ? "启用" : "禁用") << "\n";
    return oss.str();
}

void RenderManager::addCapability(const RendererCapability& cap)
{
    mCapabilities.push_back(cap);
    
    // 同时为性能统计预留空间
    RendererPerformance perf = {};
    mPerformanceStats.push_back(perf);
}

RendererCapability RenderManager::createCapability(
    rlottie::RenderBackend backend,
    const std::string& name,
    const std::string& description,
    bool available,
    bool hardwareAccel,
    int priority)
{
    RendererCapability cap;
    cap.backend = backend;
    cap.name = name;
    cap.description = description;
    cap.available = available;
    cap.hardwareAccelerated = hardwareAccel;
    cap.supportsGradients = false;
    cap.supportsMasks = false;
    cap.supportsFilters = false;
    cap.priority = priority;
    return cap;
}

// RendererFactory 实现
std::unique_ptr<VPainter> RendererFactory::createDefault()
{
    auto& manager = RenderManager::instance();
    if (!manager.initialize()) {
        return VPainter::create(RenderType::CPU);
    }
    
    return manager.createRenderer(manager.getDefaultRenderer());
}

std::unique_ptr<VPainter> RendererFactory::create(rlottie::RenderBackend backend)
{
    auto& manager = RenderManager::instance();
    manager.initialize();
    
    return manager.createRenderer(backend);
}

std::unique_ptr<VPainter> RendererFactory::createBest()
{
    auto& manager = RenderManager::instance();
    if (!manager.initialize()) {
        return VPainter::create(RenderType::CPU);
    }
    
    auto best = manager.selectBestRenderer();
    return manager.createRenderer(best);
}

bool RendererFactory::isAvailable(rlottie::RenderBackend backend)
{
    auto& manager = RenderManager::instance();
    manager.initialize();
    
    return manager.isRendererAvailable(backend);
}

rlottie::RenderBackend RendererFactory::getRecommended()
{
    auto& manager = RenderManager::instance();
    manager.initialize();
    
    return manager.selectBestRenderer();
}

std::vector<rlottie::RenderBackend> RendererFactory::listAvailable()
{
    auto& manager = RenderManager::instance();
    manager.initialize();
    
    std::vector<rlottie::RenderBackend> available;
    for (const auto& cap : manager.getCapabilities()) {
        if (cap.available) {
            available.push_back(cap.backend);
        }
    }
    
    return available;
}

V_END_NAMESPACE 