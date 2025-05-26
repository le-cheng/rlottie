/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef VPAINTER_QT_H
#define VPAINTER_QT_H

#include "vpainter.h"
#include "vpath.h"

// 检查是否启用了Qt渲染后端
#ifdef LOTTIE_QT

// Qt头文件在使用此头文件的.cpp文件中包含，这里只声明类
class QImage;
class QPainter;
class QPainterPath;
class QBrush;
class QPen;
class QRect;
class QPoint;
class QColor;
class QLinearGradient;
class QRadialGradient;

V_BEGIN_NAMESPACE

// Qt渲染后端适配器
class VPainterQt : public VPainter {
public:
    VPainterQt() = default;
    explicit VPainterQt(VBitmap *buffer);
    
    RenderType renderType() const override { return RenderType::Qt; }
    
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

    // 矢量绘制方法 - Qt的优势所在
    void drawPath(const VPath &path, const VBrush &brush) override;
    void drawPath(const VPath &path, const VBrush &brush, CapStyle cap, JoinStyle join, float width) override;

private:
    QImage        *mQImage = nullptr;
    QPainter      *mQPainter = nullptr;
    QRect         *mDrawRect = nullptr;
    QBrush        *mQBrush = nullptr;
    QPen          *mQPen = nullptr;
    VRasterBuffer  mBuffer;
    VSpanData      mSpanData;
    
    // 将VRle转换为QPainterPath
    QPainterPath rleToPath(const VRle &rle, const VPoint &pos = VPoint());
    // 将VBrush转换为QBrush
    QBrush brushToQBrush(const VBrush &brush);
    // 将BlendMode转换为QPainter::CompositionMode
    int blendModeToCompositionMode(BlendMode mode);

    QPainterPath convertVPathToQPainterPath(const VPath &path);
    void setupQPainter(const VBrush &brush);
};

V_END_NAMESPACE

#endif // LOTTIE_QT

#endif // VPAINTER_QT_H 