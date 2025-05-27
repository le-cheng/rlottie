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

#ifndef VPAINTER_VGLITE_H
#define VPAINTER_VGLITE_H

#include "vpainter.h"
#include "vpath.h"

// 检查是否启用了VGLite渲染后端
#ifdef LOTTIE_VGLITE

// VGLite头文件
extern "C" {
#include "vg_lite.h"
}

V_BEGIN_NAMESPACE

// VGLite渲染后端适配器
class VPainterVGLite : public VPainter {
public:
    VPainterVGLite() = default;
    explicit VPainterVGLite(VBitmap *buffer);
    virtual ~VPainterVGLite();
    
    RenderType renderType() const override { return RenderType::VGLite; }
    
    bool begin(VBitmap *buffer) override;
    void end() override;
    void setDrawRegion(const VRect &region) override;
    void setBrush(const VBrush &brush) override;
    void setBlendMode(BlendMode mode) override;
    void drawRle(const VPoint &pos, const VRle &rle) override;
    void drawRle(const VRle &rle, const VRle &clip) override;
    VRect clipBoundingRect() const override;
    void drawBitmap(const VPoint &point, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha = 255) override;
    void drawBitmap(const VRect &target, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha = 255) override;
    void drawBitmap(const VPoint &point, const VBitmap &bitmap, uint8_t const_alpha = 255) override;
    void drawBitmap(const VRect &rect, const VBitmap &bitmap, uint8_t const_alpha = 255) override;

    // 矢量绘制方法 - VGLite的GPU加速优势
    void drawPath(const VPath &path, const VBrush &brush) override;
    void drawPath(const VPath &path, const VBrush &brush, CapStyle cap, JoinStyle join, float width) override;

    // VGLite特有的功能
    void flush();
    void clearBuffer();
    void clearBuffer(const VRect &region);
    
    // 获取VGLite错误信息
    static const char* getErrorString(vg_lite_error_t error);

private:
    vg_lite_buffer_t      mVGBuffer{};
    vg_lite_path_t       *mVGPath = nullptr;
    vg_lite_linear_gradient_t mLinearGrad{};
    vg_lite_radial_gradient_t mRadialGrad{};
    vg_lite_color_t       mCurrentColor = 0;
    vg_lite_blend_t       mBlendMode = VG_LITE_BLEND_SRC_OVER;
    vg_lite_fill_t        mFillRule = VG_LITE_FILL_NON_ZERO;
    vg_lite_matrix_t      mMatrix{};
    VRect                 mClipRect;
    VRasterBuffer         mBuffer;
    VSpanData             mSpanData;
    bool                  mVGInitialized = false;
    
    // 内部辅助方法
    bool initializeVGLite();
    void cleanupVGLite();
    vg_lite_path_t* convertVPathToVGPath(const VPath &path);
    void setupVGBrush(const VBrush &brush);
    vg_lite_blend_t convertBlendMode(BlendMode mode);
    vg_lite_cap_style_t convertCapStyle(CapStyle cap);
    vg_lite_join_style_t convertJoinStyle(JoinStyle join);
    void releaseVGPath(vg_lite_path_t *path);
    
    // RLE到VGLite路径转换
    vg_lite_path_t* rleToVGPath(const VRle &rle, const VPoint &pos = VPoint());
    
    // 错误处理
    bool checkVGError(vg_lite_error_t error, const char* operation);
};

V_END_NAMESPACE

#endif // LOTTIE_VGLITE

#endif // VPAINTER_VGLITE_H 