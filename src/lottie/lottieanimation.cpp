/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

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
#include "config.h"
#include "lottieitem.h"
#include "lottiemodel.h"
#include "rlottie.h"

#include <fstream>

using namespace rlottie;
using namespace rlottie::internal;

// 全局默认渲染后端
static RenderBackend gDefaultRenderBackend = RenderBackend::CPU;

RLOTTIE_API void rlottie::configureRenderBackend(RenderBackend backend)
{
    gDefaultRenderBackend = backend;
}

RLOTTIE_API void rlottie::configureModelCacheSize(size_t cacheSize)
{
    internal::model::configureModelCacheSize(cacheSize);
}

struct RenderTask {
    RenderTask() { receiver = sender.get_future(); }
    std::promise<Surface> sender;
    std::future<Surface>  receiver;
    AnimationImpl *       playerImpl{nullptr};
    size_t                frameNo{0};
    Surface               surface;
    bool                  keepAspectRatio{true};
};
using SharedRenderTask = std::shared_ptr<RenderTask>;

class AnimationImpl {
public:
    void    init(std::shared_ptr<model::Composition> composition);
    bool    update(size_t frameNo, const VSize &size, bool keepAspectRatio);
    VSize   size() const { return mModel->size(); }
    double  duration() const { return mModel->duration(); }
    double  frameRate() const { return mModel->frameRate(); }
    size_t  totalFrame() const { return mModel->totalFrame(); }
    size_t  frameAtPos(double pos) const { return mModel->frameAtPos(pos); }
    Surface render(size_t frameNo, const Surface &surface,
                   bool keepAspectRatio);
    std::future<Surface> renderAsync(size_t frameNo, Surface &&surface,
                                     bool keepAspectRatio);
    const LOTLayerNode * renderTree(size_t frameNo, const VSize &size);

    const LayerInfoList &layerInfoList() const
    {
        if (mLayerList.empty()) {
            mLayerList = mModel->layerInfoList();
        }
        return mLayerList;
    }
    const MarkerList &markers() const { return mModel->markers(); }
    void              setValue(const std::string &keypath, LOTVariant &&value);
    void              removeFilter(const std::string &keypath, Property prop);
    
    // 设置渲染后端
    void setRenderBackend(RenderBackend backend) 
    {
        mRenderer->setRenderBackend(static_cast<RenderType>(backend));
    }
    
    // 获取当前渲染后端
    RenderBackend renderBackend() const 
    {
        return static_cast<RenderBackend>(mRenderer->renderBackend());
    }

private:
    mutable LayerInfoList                  mLayerList;
    model::Composition *                   mModel;
    SharedRenderTask                       mTask;
    std::unique_ptr<renderer::Composition> mRenderer{nullptr};
};

void AnimationImpl::setValue(const std::string &keypath, LOTVariant &&value)
{
    if (keypath.empty()) return;
    mRenderer->setValue(keypath, value);
}

const LOTLayerNode *AnimationImpl::renderTree(size_t frameNo, const VSize &size)
{
    if (update(frameNo, size, true)) {
        mRenderer->buildRenderTree();
    }
    return mRenderer->renderTree();
}

bool AnimationImpl::update(size_t frameNo, const VSize &size,
                           bool keepAspectRatio)
{
    frameNo += mModel->startFrame();

    if (frameNo > mModel->endFrame()) frameNo = mModel->endFrame();

    if (frameNo < mModel->startFrame()) frameNo = mModel->startFrame();

    return mRenderer->update(int(frameNo), size, keepAspectRatio);
}

Surface AnimationImpl::render(size_t frameNo, const Surface &surface,
                              bool keepAspectRatio)
{
    update(
        frameNo,
        VSize(int(surface.drawRegionWidth()), int(surface.drawRegionHeight())),
        keepAspectRatio);
    mRenderer->render(surface);

    return surface;
}

void AnimationImpl::init(std::shared_ptr<model::Composition> composition)
{
    mModel = composition.get();
    mRenderer = std::make_unique<renderer::Composition>(composition);
    
    // 设置为全局默认渲染后端
    if (gDefaultRenderBackend != RenderBackend::CPU) {
        setRenderBackend(gDefaultRenderBackend);
    }
}

class RenderTaskScheduler {
public:
    static bool IsRunning;

    static RenderTaskScheduler &instance()
    {
        static RenderTaskScheduler singleton;
        return singleton;
    }

    void stop() {}

    std::future<Surface> process(SharedRenderTask task)
    {
        auto result = task->playerImpl->render(task->frameNo, task->surface,
                                               task->keepAspectRatio);
        task->sender.set_value(result);
        return std::move(task->receiver);
    }
};

bool RenderTaskScheduler::IsRunning{false};

std::future<Surface> AnimationImpl::renderAsync(size_t    frameNo,
                                                Surface &&surface,
                                                bool      keepAspectRatio)
{
    if (!mTask) {
        mTask = std::make_shared<RenderTask>();
    } else {
        mTask->sender = std::promise<Surface>();
        mTask->receiver = mTask->sender.get_future();
    }
    mTask->playerImpl = this;
    mTask->frameNo = frameNo;
    mTask->surface = std::move(surface);
    mTask->keepAspectRatio = keepAspectRatio;

    return RenderTaskScheduler::instance().process(mTask);
}

/**
 * \breif Brief abput the Api.
 * Description about the setFilePath Api
 * @param path  add the details
 */
std::unique_ptr<Animation> Animation::loadFromData(
    std::string jsonData, const std::string &key,
    const std::string &resourcePath, bool cachePolicy)
{
    if (jsonData.empty()) {
        vWarning << "jason data is empty";
        return nullptr;
    }

    auto composition = model::loadFromData(std::move(jsonData), key,
                                           resourcePath, cachePolicy);
    if (composition) {
        auto animation = std::unique_ptr<Animation>(new Animation);
        animation->d->init(std::move(composition));
        return animation;
    }

    return nullptr;
}

std::unique_ptr<Animation> Animation::loadFromData(std::string jsonData,
                                                   std::string resourcePath,
                                                   ColorFilter filter)
{
    if (jsonData.empty()) {
        vWarning << "jason data is empty";
        return nullptr;
    }

    auto composition = model::loadFromData(
        std::move(jsonData), std::move(resourcePath), std::move(filter));
    if (composition) {
        auto animation = std::unique_ptr<Animation>(new Animation);
        animation->d->init(std::move(composition));
        return animation;
    }
    return nullptr;
}

std::unique_ptr<Animation> Animation::loadFromFile(const std::string &path,
                                                   bool cachePolicy)
{
    if (path.empty()) {
        vWarning << "File path is empty";
        return nullptr;
    }

    auto composition = model::loadFromFile(path, cachePolicy);
    if (composition) {
        auto animation = std::unique_ptr<Animation>(new Animation);
        animation->d->init(std::move(composition));
        return animation;
    }
    return nullptr;
}

void Animation::size(size_t &width, size_t &height) const
{
    VSize sz = d->size();

    width = sz.width();
    height = sz.height();
}

double Animation::duration() const
{
    return d->duration();
}

double Animation::frameRate() const
{
    return d->frameRate();
}

size_t Animation::totalFrame() const
{
    return d->totalFrame();
}

size_t Animation::frameAtPos(double pos)
{
    return d->frameAtPos(pos);
}

const LOTLayerNode *Animation::renderTree(size_t frameNo, size_t width,
                                          size_t height) const
{
    return d->renderTree(frameNo, VSize(int(width), int(height)));
}

std::future<Surface> Animation::render(size_t frameNo, Surface surface,
                                       bool keepAspectRatio)
{
    return d->renderAsync(frameNo, std::move(surface), keepAspectRatio);
}

void Animation::renderSync(size_t frameNo, Surface surface,
                           bool keepAspectRatio)
{
    d->render(frameNo, surface, keepAspectRatio);
}

const LayerInfoList &Animation::layers() const
{
    return d->layerInfoList();
}

const MarkerList &Animation::markers() const
{
    return d->markers();
}

void Animation::setValue(Color_Type, Property prop, const std::string &keypath,
                         Color value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Float_Type, Property prop, const std::string &keypath,
                         float value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Size_Type, Property prop, const std::string &keypath,
                         Size value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Point_Type, Property prop, const std::string &keypath,
                         Point value)
{
    d->setValue(keypath,
                LOTVariant(prop, [value](const FrameInfo &) { return value; }));
}

void Animation::setValue(Color_Type, Property prop, const std::string &keypath,
                         std::function<Color(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

void Animation::setValue(Float_Type, Property prop, const std::string &keypath,
                         std::function<float(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

void Animation::setValue(Size_Type, Property prop, const std::string &keypath,
                         std::function<Size(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

void Animation::setValue(Point_Type, Property prop, const std::string &keypath,
                         std::function<Point(const FrameInfo &)> &&value)
{
    d->setValue(keypath, LOTVariant(prop, value));
}

Animation::~Animation() = default;
Animation::Animation() : d(std::make_unique<AnimationImpl>()) {}

Surface::Surface(uint32_t *buffer, size_t width, size_t height,
                 size_t bytesPerLine)
    : mBuffer(buffer),
      mWidth(width),
      mHeight(height),
      mBytesPerLine(bytesPerLine)
{
    mDrawArea.w = mWidth;
    mDrawArea.h = mHeight;
}

void Surface::setDrawRegion(size_t x, size_t y, size_t width, size_t height)
{
    if ((x + width > mWidth) || (y + height > mHeight)) return;

    mDrawArea.x = x;
    mDrawArea.y = y;
    mDrawArea.w = width;
    mDrawArea.h = height;
}

// private apis exposed to c interface
void lottie_init_impl()
{
    // do nothing for now.
}

extern void lottieShutdownRasterTaskScheduler();

void lottie_shutdown_impl()
{
    lottieShutdownRasterTaskScheduler();
}

#ifdef LOTTIE_LOGGING_SUPPORT
void initLogging()
{
#if defined(__ARM_NEON__)
    set_log_level(LogLevel::OFF);
#else
    initialize(GuaranteedLogger{}, "/home/lecheng/workspace/e_lottie/rlottie/out/", "rlottie", 1);
    set_log_level(LogLevel::INFO);
#endif
}

V_CONSTRUCTOR_FUNCTION(initLogging)
#endif

void Animation::setRenderBackend(RenderBackend backend)
{
    d->setRenderBackend(backend);
}

RenderBackend Animation::renderBackend() const
{
    return d->renderBackend();
}
