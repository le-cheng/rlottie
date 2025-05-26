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

#ifndef VPAINTER_H
#define VPAINTER_H

#include "vbrush.h"
#include "vpoint.h"
#include "vrle.h"
#include "vdrawhelper.h"
#include <memory>

V_BEGIN_NAMESPACE

// 渲染后端类型(与rlottie::RenderBackend保持一致)
enum class RenderType {
    CPU,    // 默认CPU软件渲染
    Qt,     // Qt渲染
    Custom  // 自定义渲染
};

class VBitmap;
class VPainter {
public:
    virtual ~VPainter() = default;
    
    // 创建特定类型的渲染器
    static std::unique_ptr<VPainter> create(RenderType type = RenderType::CPU);
    
    // 抽象接口
    virtual bool  begin(VBitmap *buffer) = 0;
    virtual void  end() = 0;
    virtual void  setDrawRegion(const VRect &region) = 0; // sub surface rendering area.
    virtual void  setBrush(const VBrush &brush) = 0;
    virtual void  setBlendMode(BlendMode mode) = 0;
    virtual void  drawRle(const VPoint &pos, const VRle &rle) = 0;
    virtual void  drawRle(const VRle &rle, const VRle &clip) = 0;
    virtual VRect clipBoundingRect() const = 0;

    virtual void  drawBitmap(const VPoint &point, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha = 255) = 0;
    virtual void  drawBitmap(const VRect &target, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha = 255) = 0;
    virtual void  drawBitmap(const VPoint &point, const VBitmap &bitmap, uint8_t const_alpha = 255) = 0;
    virtual void  drawBitmap(const VRect &rect, const VBitmap &bitmap, uint8_t const_alpha = 255) = 0;
};

// 默认CPU渲染器实现
class VPainterCPU : public VPainter {
public:
    VPainterCPU() = default;
    explicit VPainterCPU(VBitmap *buffer);
    bool  begin(VBitmap *buffer) override;
    void  end() override;
    void  setDrawRegion(const VRect &region) override;
    void  setBrush(const VBrush &brush) override;
    void  setBlendMode(BlendMode mode) override;
    void  drawRle(const VPoint &pos, const VRle &rle) override;
    void  drawRle(const VRle &rle, const VRle &clip) override;
    VRect clipBoundingRect() const override;

    void  drawBitmap(const VPoint &point, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha = 255) override;
    void  drawBitmap(const VRect &target, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha = 255) override;
    void  drawBitmap(const VPoint &point, const VBitmap &bitmap, uint8_t const_alpha = 255) override;
    void  drawBitmap(const VRect &rect, const VBitmap &bitmap, uint8_t const_alpha = 255) override;
private:
    void drawBitmapUntransform(const VRect &target, const VBitmap &bitmap,
                               const VRect &source, uint8_t const_alpha);
    VRasterBuffer mBuffer;
    VSpanData     mSpanData;
};

V_END_NAMESPACE

#endif  // VPAINTER_H
