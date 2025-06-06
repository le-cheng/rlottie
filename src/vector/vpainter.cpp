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

// 必须首先包含config.h以确保宏定义正确
#if defined(__has_include)
#  if __has_include("config.h")
#    include "config.h"
#  endif
#endif

// 然后包含vglobal.h确保命名空间正确定义
#include "vglobal.h"

// 再包含其他头文件
#include "vpainter.h"
#include "vpath.h"
#include "vraster.h"
#include <algorithm>
#include <memory>
#include "vdebug.h"

// 如果C++14的std::make_unique不可用，提供自己的实现
#if __cplusplus < 201402L
namespace std {
    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args) {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif

// 引入Qt渲染器头文件，处理条件编译
#if defined(LOTTIE_QT) || defined(__has_include)
#  if defined(LOTTIE_QT) || __has_include("vpainter_qt.h")
#    include "vpainter_qt.h"
#    define HAS_QT_PAINTER 1
#  endif
#endif

#if defined(LOTTIE_VGLITE) || defined(__has_include)
#  if defined(LOTTIE_VGLITE) || __has_include("vpainter_vglite.h")
#    include "vpainter_vglite.h"
#    define HAS_VGLITE_PAINTER 1
#  endif
#endif

V_BEGIN_NAMESPACE

// 创建特定类型的渲染器实例
std::unique_ptr<VPainter> VPainter::create(RenderType type)
{
    switch (type) {
    case RenderType::CPU:
        return std::unique_ptr<VPainter>(static_cast<VPainter*>(new VPainterCPU()));
    case RenderType::Qt:
#ifdef HAS_QT_PAINTER
        return std::unique_ptr<VPainter>(static_cast<VPainter*>(new VPainterQt()));
#else
        vDebug << "Qt渲染器未编译，回退到CPU渲染";
        return std::unique_ptr<VPainter>(static_cast<VPainter*>(new VPainterCPU()));
#endif
    case RenderType::VGLite:
        // VGLite渲染器开发中，暂时回退到CPU渲染
        vDebug << "VGLite渲染器开发中，回退到CPU渲染";
        return std::unique_ptr<VPainter>(static_cast<VPainter*>(new VPainterCPU()));
    case RenderType::OpenGL:
        vDebug << "OpenGL渲染器待实现，回退到CPU渲染";
        return std::unique_ptr<VPainter>(static_cast<VPainter*>(new VPainterCPU()));
    case RenderType::Vulkan:
        vDebug << "Vulkan渲染器待实现，回退到CPU渲染";
        return std::unique_ptr<VPainter>(static_cast<VPainter*>(new VPainterCPU()));
    case RenderType::Custom:
    default:
        // 默认返回CPU渲染器
        return std::unique_ptr<VPainter>(static_cast<VPainter*>(new VPainterCPU()));
    }
}

// VPainterCPU实现
void VPainterCPU::drawRle(const VPoint &, const VRle &rle)
{
    if (rle.empty()) return;
    // mSpanData.updateSpanFunc();

    if (!mSpanData.mUnclippedBlendFunc) return;

    // do draw after applying clip.
    rle.intersect(mSpanData.clipRect(), mSpanData.mUnclippedBlendFunc,
                  &mSpanData);
}

void VPainterCPU::drawRle(const VRle &rle, const VRle &clip)
{
    if (rle.empty() || clip.empty()) return;

    if (!mSpanData.mUnclippedBlendFunc) return;

    rle.intersect(clip, mSpanData.mUnclippedBlendFunc, &mSpanData);
}

static void fillRect(const VRect &r, VSpanData *data)
{
    auto x1 = std::max(r.x(), 0);
    auto x2 = std::min(r.x() + r.width(), data->mDrawableSize.width());
    auto y1 = std::max(r.y(), 0);
    auto y2 = std::min(r.y() + r.height(), data->mDrawableSize.height());

    if (x2 <= x1 || y2 <= y1) return;

    const int  nspans = 256;
    VRle::Span spans[nspans];

    int y = y1;
    while (y < y2) {
        int n = std::min(nspans, y2 - y);
        int i = 0;
        while (i < n) {
            spans[i].x = short(x1);
            spans[i].len = uint16_t(x2 - x1);
            spans[i].y = short(y + i);
            spans[i].coverage = 255;
            ++i;
        }

        data->mUnclippedBlendFunc(n, spans, data);
        y += n;
    }
}

void VPainterCPU::drawBitmapUntransform(const VRect &  target,
                                         const VBitmap &bitmap,
                                         const VRect &  source,
                                         uint8_t        const_alpha)
{
    mSpanData.initTexture(&bitmap, const_alpha, source);
    if (!mSpanData.mUnclippedBlendFunc) return;

    // update translation matrix for source texture.
    mSpanData.dx = float(target.x() - source.x());
    mSpanData.dy = float(target.y() - source.y());

    fillRect(target, &mSpanData);
}

VPainterCPU::VPainterCPU(VBitmap *buffer)
{
    begin(buffer);
}

bool VPainterCPU::begin(VBitmap *buffer)
{
    mBuffer.prepare(buffer);
    mSpanData.init(&mBuffer);
    // TODO find a better api to clear the surface
    mBuffer.clear();
    return true;
}

void VPainterCPU::end() {}

void VPainterCPU::setDrawRegion(const VRect &region)
{
    mSpanData.setDrawRegion(region);
}

void VPainterCPU::setBrush(const VBrush &brush)
{
    mSpanData.setup(brush);
}

void VPainterCPU::setBlendMode(BlendMode mode)
{
    mSpanData.mBlendMode = mode;
}

VRect VPainterCPU::clipBoundingRect() const
{
    return mSpanData.clipRect();
}

void VPainterCPU::drawBitmap(const VPoint &point, const VBitmap &bitmap,
                          const VRect &source, uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    drawBitmap(VRect(point, bitmap.size()),
               bitmap, source, const_alpha);
}

void VPainterCPU::drawBitmap(const VRect &target, const VBitmap &bitmap,
                          const VRect &source, uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    // clear any existing brush data.
    setBrush(VBrush());

    if (target.size() == source.size()) {
        drawBitmapUntransform(target, bitmap, source, const_alpha);
    } else {
        // @TODO scaling
    }
}

void VPainterCPU::drawBitmap(const VPoint &point, const VBitmap &bitmap,
                          uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    drawBitmap(VRect(point, bitmap.size()),
               bitmap, bitmap.rect(),
               const_alpha);
}

void VPainterCPU::drawBitmap(const VRect &rect, const VBitmap &bitmap,
                          uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    drawBitmap(rect, bitmap, bitmap.rect(),
               const_alpha);
}

// VPainterCPU的drawPath实现 - 转换为RLE后绘制
void VPainterCPU::drawPath(const VPath &path, const VBrush &brush)
{
    if (path.empty()) return;
    
    // 设置画刷
    setBrush(brush);
    
    // 创建光栅化器并转换VPath为VRle
    VRasterizer rasterizer;
    rasterizer.rasterize(path, FillRule::Winding, clipBoundingRect());
    VRle rle = rasterizer.rle();
    
    // 绘制RLE
    drawRle(VPoint(), rle);
}

void VPainterCPU::drawPath(const VPath &path, const VBrush &brush, CapStyle cap, JoinStyle join, float width)
{
    if (path.empty()) return;
    
    // 设置画刷
    setBrush(brush);
    
    // 创建光栅化器并转换VPath为VRle(描边)
    VRasterizer rasterizer;
    rasterizer.rasterize(path, cap, join, width, 1.0f, clipBoundingRect());
    VRle rle = rasterizer.rle();
    
    // 绘制RLE
    drawRle(VPoint(), rle);
}

V_END_NAMESPACE
