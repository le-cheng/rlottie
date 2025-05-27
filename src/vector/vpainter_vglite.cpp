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

#ifdef LOTTIE_VGLITE

#include "vpainter_vglite.h"
#include "vdrawhelper.h"
#include "vpath.h"
#include "vdebug.h"
#include <cstring>
#include <memory>

V_BEGIN_NAMESPACE

VPainterVGLite::VPainterVGLite(VBitmap *buffer)
{
    begin(buffer);
}

VPainterVGLite::~VPainterVGLite()
{
    cleanupVGLite();
}

bool VPainterVGLite::initializeVGLite()
{
    if (mVGInitialized) return true;

    vg_lite_error_t error = vg_lite_init(0, 0);
    if (!checkVGError(error, "vg_lite_init")) {
        return false;
    }

    // 初始化变换矩阵为单位矩阵
    vg_lite_identity(&mMatrix);
    
    mVGInitialized = true;
    vDebug << "VGLite渲染器初始化成功";
    return true;
}

void VPainterVGLite::cleanupVGLite()
{
    if (mVGPath) {
        releaseVGPath(mVGPath);
        mVGPath = nullptr;
    }

    if (mVGInitialized) {
        vg_lite_close();
        mVGInitialized = false;
        vDebug << "VGLite渲染器清理完成";
    }
}

bool VPainterVGLite::begin(VBitmap *buffer)
{
    if (!initializeVGLite()) {
        vWarning << "VGLite初始化失败，回退到CPU渲染";
        return false;
    }

    mBuffer.prepare(buffer);
    mSpanData.init(&mBuffer);
    
    // 清理内存中的脏区域
    clearBuffer();
    
    // 设置VGLite缓冲区
    memset(&mVGBuffer, 0, sizeof(mVGBuffer));
    mVGBuffer.width = buffer->width();
    mVGBuffer.height = buffer->height();
    mVGBuffer.stride = buffer->stride();
    mVGBuffer.memory = buffer->data();
    mVGBuffer.address = reinterpret_cast<vg_lite_uint32_t>(buffer->data());
    
    // 根据位图格式设置VGLite格式
    switch (buffer->format()) {
    case VBitmap::Format::ARGB32_Premultiplied:
        mVGBuffer.format = VG_LITE_BGRA8888;
        break;
    case VBitmap::Format::ARGB32:
        mVGBuffer.format = VG_LITE_ABGR8888;
        break;
    default:
        mVGBuffer.format = VG_LITE_BGRA8888;
        break;
    }

    // 设置默认参数
    mCurrentColor = 0xFF000000; // 黑色，完全不透明
    mBlendMode = VG_LITE_BLEND_SRC_OVER;
    mFillRule = VG_LITE_FILL_NON_ZERO;
    mClipRect = VRect(0, 0, buffer->width(), buffer->height());

    vDebug << "VGLite渲染器开始，缓冲区尺寸：" << buffer->width() << "x" << buffer->height();
    return true;
}

void VPainterVGLite::end()
{
    flush();
    vDebug << "VGLite渲染器结束";
}

void VPainterVGLite::setDrawRegion(const VRect &region)
{
    mClipRect = region;
    mSpanData.setDrawRegion(region);
    
    // 设置VGLite裁剪区域
    vg_lite_error_t error = vg_lite_set_scissor(
        region.left(), region.top(), 
        region.width(), region.height()
    );
    checkVGError(error, "vg_lite_set_scissor");
}

void VPainterVGLite::setBrush(const VBrush &brush)
{
    mSpanData.setup(brush);
    setupVGBrush(brush);
}

void VPainterVGLite::setupVGBrush(const VBrush &brush)
{
    switch (brush.type()) {
    case VBrush::Type::Solid: {
        VColor color = brush.color();
        mCurrentColor = ((color.alpha() & 0xFF) << 24) |
                       ((color.red() & 0xFF) << 16) |
                       ((color.green() & 0xFF) << 8) |
                       (color.blue() & 0xFF);
        break;
    }
    case VBrush::Type::LinearGradient: {
        // TODO: 实现线性渐变
        vDebug << "VGLite线性渐变待实现";
        mCurrentColor = 0xFF808080; // 临时使用灰色
        break;
    }
    case VBrush::Type::RadialGradient: {
        // TODO: 实现径向渐变
        vDebug << "VGLite径向渐变待实现";
        mCurrentColor = 0xFF808080; // 临时使用灰色
        break;
    }
    default:
        mCurrentColor = 0xFF000000; // 默认黑色
        break;
    }
}

void VPainterVGLite::setBlendMode(BlendMode mode)
{
    mSpanData.mBlendMode = mode;
    mBlendMode = convertBlendMode(mode);
}

vg_lite_blend_t VPainterVGLite::convertBlendMode(BlendMode mode)
{
    switch (mode) {
    case BlendMode::Src:
        return VG_LITE_BLEND_SRC;
    case BlendMode::SrcOver:
        return VG_LITE_BLEND_SRC_OVER;
    case BlendMode::DestIn:
        return VG_LITE_BLEND_DST_IN;
    case BlendMode::DestOut:
        return VG_LITE_BLEND_DST_OUT;
    default:
        return VG_LITE_BLEND_SRC_OVER;
    }
}

void VPainterVGLite::drawRle(const VPoint &pos, const VRle &rle)
{
    if (rle.empty()) return;

    vg_lite_path_t *path = rleToVGPath(rle, pos);
    if (!path) return;

    vg_lite_error_t error = vg_lite_draw(
        &mVGBuffer, path, mFillRule, &mMatrix, 
        mBlendMode, mCurrentColor
    );
    checkVGError(error, "vg_lite_draw");

    releaseVGPath(path);
}

void VPainterVGLite::drawRle(const VRle &rle, const VRle &clip)
{
    // TODO: 实现带裁剪的RLE绘制
    // 暂时简化处理，直接绘制rle
    drawRle(VPoint(), rle);
}

VRect VPainterVGLite::clipBoundingRect() const
{
    return mClipRect;
}

void VPainterVGLite::drawBitmap(const VPoint &point, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha)
{
    // TODO: 实现VGLite位图绘制
    vWarning << "VGLite位图绘制待实现";
}

void VPainterVGLite::drawBitmap(const VRect &target, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha)
{
    // TODO: 实现VGLite位图绘制
    vWarning << "VGLite位图绘制待实现";
}

void VPainterVGLite::drawBitmap(const VPoint &point, const VBitmap &bitmap, uint8_t const_alpha)
{
    // TODO: 实现VGLite位图绘制
    vWarning << "VGLite位图绘制待实现";
}

void VPainterVGLite::drawBitmap(const VRect &rect, const VBitmap &bitmap, uint8_t const_alpha)
{
    // TODO: 实现VGLite位图绘制
    vWarning << "VGLite位图绘制待实现";
}

void VPainterVGLite::drawPath(const VPath &path, const VBrush &brush)
{
    if (path.empty()) return;

    setupVGBrush(brush);
    
    vg_lite_path_t *vgPath = convertVPathToVGPath(path);
    if (!vgPath) return;

    vg_lite_error_t error = vg_lite_draw(
        &mVGBuffer, vgPath, mFillRule, &mMatrix, 
        mBlendMode, mCurrentColor
    );
    checkVGError(error, "vg_lite_draw path");

    releaseVGPath(vgPath);
}

void VPainterVGLite::drawPath(const VPath &path, const VBrush &brush, CapStyle cap, JoinStyle join, float width)
{
    if (path.empty()) return;

    setupVGBrush(brush);
    
    vg_lite_path_t *vgPath = convertVPathToVGPath(path);
    if (!vgPath) return;

    // 转换线条样式
    vg_lite_cap_style_t vgCap = convertCapStyle(cap);
    vg_lite_join_style_t vgJoin = convertJoinStyle(join);

    vg_lite_error_t error = vg_lite_draw_stroke(
        &mVGBuffer, vgPath, vgCap, vgJoin, width, 
        &mMatrix, mBlendMode, mCurrentColor
    );
    checkVGError(error, "vg_lite_draw_stroke");

    releaseVGPath(vgPath);
}

void VPainterVGLite::flush()
{
    if (mVGInitialized) {
        vg_lite_error_t error = vg_lite_flush();
        checkVGError(error, "vg_lite_flush");
    }
}

void VPainterVGLite::clearBuffer()
{
    if (mVGInitialized) {
        vg_lite_error_t error = vg_lite_clear(&mVGBuffer, nullptr, 0x00000000);
        checkVGError(error, "vg_lite_clear");
    } else {
        mBuffer.clear();
    }
}

void VPainterVGLite::clearBuffer(const VRect &region)
{
    if (mVGInitialized) {
        vg_lite_rectangle_t rect = {
            .x = region.left(),
            .y = region.top(),
            .width = region.width(),
            .height = region.height()
        };
        vg_lite_error_t error = vg_lite_clear(&mVGBuffer, &rect, 0x00000000);
        checkVGError(error, "vg_lite_clear region");
    }
}

vg_lite_path_t* VPainterVGLite::convertVPathToVGPath(const VPath &path)
{
    // TODO: 实现VPath到vg_lite_path_t的转换
    // 这是一个复杂的转换过程，需要将VPath的元素转换为VGLite路径命令
    vWarning << "VPath到VGLite路径转换待实现";
    return nullptr;
}

vg_lite_path_t* VPainterVGLite::rleToVGPath(const VRle &rle, const VPoint &pos)
{
    // TODO: 实现RLE到vg_lite_path_t的转换
    // 这需要将RLE区域转换为路径描述
    vWarning << "RLE到VGLite路径转换待实现";
    return nullptr;
}

void VPainterVGLite::releaseVGPath(vg_lite_path_t *path)
{
    if (path && path->path) {
        vg_lite_clear_path(path);
        delete path;
    }
}

vg_lite_cap_style_t VPainterVGLite::convertCapStyle(CapStyle cap)
{
    switch (cap) {
    case CapStyle::Flat:
        return VG_LITE_CAP_BUTT;
    case CapStyle::Round:
        return VG_LITE_CAP_ROUND;
    case CapStyle::Square:
        return VG_LITE_CAP_SQUARE;
    default:
        return VG_LITE_CAP_BUTT;
    }
}

vg_lite_join_style_t VPainterVGLite::convertJoinStyle(JoinStyle join)
{
    switch (join) {
    case JoinStyle::Miter:
        return VG_LITE_JOIN_MITER;
    case JoinStyle::Round:
        return VG_LITE_JOIN_ROUND;
    case JoinStyle::Bevel:
        return VG_LITE_JOIN_BEVEL;
    default:
        return VG_LITE_JOIN_MITER;
    }
}

bool VPainterVGLite::checkVGError(vg_lite_error_t error, const char* operation)
{
    if (error != VG_LITE_SUCCESS) {
        vWarning << "VGLite错误在" << operation << ":" << getErrorString(error);
        return false;
    }
    return true;
}

const char* VPainterVGLite::getErrorString(vg_lite_error_t error)
{
    switch (error) {
    case VG_LITE_SUCCESS:
        return "成功";
    case VG_LITE_INVALID_ARGUMENT:
        return "无效参数";
    case VG_LITE_OUT_OF_MEMORY:
        return "内存不足";
    case VG_LITE_NO_CONTEXT:
        return "无上下文";
    case VG_LITE_TIMEOUT:
        return "超时";
    case VG_LITE_OUT_OF_RESOURCES:
        return "资源不足";
    case VG_LITE_GENERIC_IO:
        return "通用IO错误";
    case VG_LITE_NOT_SUPPORT:
        return "不支持";
    default:
        return "未知错误";
    }
}

V_END_NAMESPACE

#endif // LOTTIE_VGLITE 