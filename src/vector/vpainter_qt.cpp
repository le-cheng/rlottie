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

// 首先包含config.h
#if defined(__has_include)
#  if __has_include("config.h")
#    include "config.h"
#  endif
#endif

#ifdef LOTTIE_QT

// 包含需要的Qt头文件
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QBrush>
#include <QPen>

// 在包含其他头文件前先包含Qt头文件
#include "vpainter_qt.h"
#include "vdrawhelper.h"
#include "vpath.h"

V_BEGIN_NAMESPACE

VPainterQt::VPainterQt(VBitmap *buffer)
{
    begin(buffer);
}

bool VPainterQt::begin(VBitmap *buffer)
{
    mBuffer.prepare(buffer);
    mSpanData.init(&mBuffer);
    
    // 清理内存中的脏区域
    clearBuffer();
    
    // 创建QImage与buffer共享内存
    mQImage = new QImage(
        reinterpret_cast<uchar*>(buffer->data()),
        buffer->width(),
        buffer->height(),
        buffer->stride(),
        QImage::Format_ARGB32_Premultiplied
    );

    // 开始Qt绘制
    mQPainter = new QPainter();
    bool success = mQPainter->begin(mQImage);
    if (!success) {
        return false;
    }

    mQPainter->setRenderHint(QPainter::Antialiasing, true);
    mQPainter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    mDrawRect = new QRect();
    mQBrush = new QBrush();
    mQPen = new QPen();

    return true;
}

void VPainterQt::end()
{
    if (mQPainter) {
        mQPainter->end();
        delete mQPainter;
        mQPainter = nullptr;
    }

    if (mQImage) {
        delete mQImage;
        mQImage = nullptr;
    }

    if (mDrawRect) {
        delete mDrawRect;
        mDrawRect = nullptr;
    }

    if (mQBrush) {
        delete mQBrush;
        mQBrush = nullptr;
    }

    if (mQPen) {
        delete mQPen;
        mQPen = nullptr;
    }
}

void VPainterQt::clearBuffer()
{
    if (mQPainter) {
        QPainter::CompositionMode oldMode = mQPainter->compositionMode();
        mQPainter->setCompositionMode(QPainter::CompositionMode_Clear);
        mQPainter->fillRect(QRect(0, 0, mBuffer.width(), mBuffer.height()), 
                           QColor(0, 0, 0, 0));
        mQPainter->setCompositionMode(oldMode);
    } else {
        mBuffer.clear();
    }
}

void VPainterQt::clearBuffer(const VRect &region)
{
    // 只清理指定区域
    if (region.empty()) return;
    
    // 方法1: 使用Qt填充透明色
    if (mQPainter && mQImage) {
        QPainter::CompositionMode oldMode = mQPainter->compositionMode();
        mQPainter->setCompositionMode(QPainter::CompositionMode_Clear);
        mQPainter->fillRect(QRect(region.left(), region.top(), region.width(), region.height()), 
                           QColor(0, 0, 0, 0));
        mQPainter->setCompositionMode(oldMode);
    } else {
        // 方法2: 直接操作内存 - 当QPainter还未初始化时
        for (int y = region.top(); y < region.bottom(); ++y) {
            if (y >= 0 && y < static_cast<int>(mBuffer.height())) {
                uint32_t *line = reinterpret_cast<uint32_t*>(mBuffer.scanLine(y));
                int startX = std::max(0, region.left());
                int endX = std::min(static_cast<int>(mBuffer.width()), region.right());
                if (startX < endX) {
                    memset(line + startX, 0, (endX - startX) * sizeof(uint32_t));
                }
            }
        }
    }
}

void VPainterQt::setDrawRegion(const VRect &region)
{
    *mDrawRect = QRect(region.left(), region.top(), region.width(), region.height());
    mQPainter->setClipRect(*mDrawRect);
    mSpanData.setDrawRegion(region);
}

void VPainterQt::setBrush(const VBrush &brush)
{
    *mQBrush = brushToQBrush(brush);
    mQPainter->setBrush(*mQBrush);
    mSpanData.setup(brush);
}

void VPainterQt::setBlendMode(BlendMode mode)
{
    mQPainter->setCompositionMode(static_cast<QPainter::CompositionMode>(blendModeToCompositionMode(mode)));
    mSpanData.mBlendMode = mode;
}

void VPainterQt::drawRle(const VPoint &pos, const VRle &rle)
{
    if (rle.empty()) return;

    QPainterPath path = rleToPath(rle, pos);
    mQPainter->fillPath(path, *mQBrush);
}

void VPainterQt::drawRle(const VRle &rle, const VRle &clip)
{
    // TODO: 实现更复杂的RLE裁剪绘制
    // 暂时简化处理，直接绘制rle
    drawRle(VPoint(), rle);
}

VRect VPainterQt::clipBoundingRect() const
{
    QRect clipRect = mQPainter->clipBoundingRect().toRect();
    return VRect(clipRect.left(), clipRect.top(), clipRect.width(), clipRect.height());
}

void VPainterQt::drawBitmap(const VPoint &point, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    QImage img(
        reinterpret_cast<uchar*>(bitmap.data()),
        bitmap.width(),
        bitmap.height(),
        bitmap.stride(),
        QImage::Format_ARGB32_Premultiplied
    );

    QRect sourceRect(source.left(), source.top(), source.width(), source.height());

    mQPainter->setOpacity(const_alpha / 255.0);
    mQPainter->drawImage(QPoint(point.x(), point.y()), img, sourceRect);
    mQPainter->setOpacity(1.0);
}

void VPainterQt::drawBitmap(const VRect &target, const VBitmap &bitmap, const VRect &source, uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    QImage img(
        reinterpret_cast<uchar*>(bitmap.data()),
        bitmap.width(),
        bitmap.height(),
        bitmap.stride(),
        QImage::Format_ARGB32_Premultiplied
    );

    QRect sourceRect(source.left(), source.top(), source.width(), source.height());
    QRect targetRect(target.left(), target.top(), target.width(), target.height());

    mQPainter->setOpacity(const_alpha / 255.0);
    mQPainter->drawImage(targetRect, img, sourceRect);
    mQPainter->setOpacity(1.0);
}

void VPainterQt::drawBitmap(const VPoint &point, const VBitmap &bitmap, uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    drawBitmap(VRect(point.x(), point.y(), bitmap.width(), bitmap.height()),
               bitmap, bitmap.rect(), const_alpha);
}

void VPainterQt::drawBitmap(const VRect &rect, const VBitmap &bitmap, uint8_t const_alpha)
{
    if (!bitmap.valid()) return;

    drawBitmap(rect, bitmap, bitmap.rect(), const_alpha);
}

// 静态函数处理span，用于VRle::intersect
struct PathDataContext {
    QPainterPath *path;
    VPoint pos;
};

static void processSpans(size_t count, const VRle::Span *spans, void *userData) {
    PathDataContext *ctx = static_cast<PathDataContext*>(userData);
    for (size_t i = 0; i < count; i++) {
        const VRle::Span &span = spans[i];
        int x = span.x + ctx->pos.x();
        int y = span.y + ctx->pos.y();

        // 添加到路径
        ctx->path->addRect(x, y, span.len, 1);
    }
}

QPainterPath VPainterQt::rleToPath(const VRle &rle, const VPoint &pos)
{
    QPainterPath path;

    PathDataContext ctx;
    ctx.path = &path;
    ctx.pos = pos;

    // 使用临时的VRect调用intersect处理所有spans
    VRect rect = rle.boundingRect(); // 使用RLE自己的边界矩形
    rle.intersect(rect, processSpans, &ctx);

    return path;
}

QBrush VPainterQt::brushToQBrush(const VBrush &brush)
{
    switch (brush.type()) {
    case VBrush::Type::Solid: {
        VColor color = brush.mColor;
        return QBrush(QColor(color.r, color.g, color.b, color.a));
    }
    case VBrush::Type::LinearGradient: {
        // 这里只是基本实现，实际项目中可能需要更复杂的渐变处理
        const VGradient* gradient = brush.mGradient;
        QLinearGradient qGradient;

        // 设置渐变颜色
        for (const auto &stop : gradient->mStops) {
            VColor color = stop.second;
            qGradient.setColorAt(stop.first, QColor(color.r, color.g, color.b, color.a));
        }

        return QBrush(qGradient);
    }
    case VBrush::Type::RadialGradient: {
        // 类似于线性渐变的实现
        const VGradient* gradient = brush.mGradient;
        QRadialGradient qGradient;

        for (const auto &stop : gradient->mStops) {
            VColor color = stop.second;
            qGradient.setColorAt(stop.first, QColor(color.r, color.g, color.b, color.a));
        }

        return QBrush(qGradient);
    }
    case VBrush::Type::Texture: {
        // 纹理处理
        const VTexture* texture = brush.mTexture;
        if (!texture) return QBrush();

        QImage img(
            reinterpret_cast<uchar*>(texture->mBitmap.data()),
            texture->mBitmap.width(),
            texture->mBitmap.height(),
            texture->mBitmap.stride(),
            QImage::Format_ARGB32_Premultiplied
        );

        return QBrush(img);
    }
    default:
        return QBrush();
    }
}

int VPainterQt::blendModeToCompositionMode(BlendMode mode)
{
    switch (mode) {
    case BlendMode::Src:
        return QPainter::CompositionMode_Source;
    case BlendMode::SrcOver:
        return QPainter::CompositionMode_SourceOver;
    case BlendMode::DestIn:
        return QPainter::CompositionMode_DestinationIn;
    case BlendMode::DestOut:
        return QPainter::CompositionMode_DestinationOut;
    default:
        return QPainter::CompositionMode_SourceOver;
    }
}

// 实现真正的矢量绘制 - Qt的优势
void VPainterQt::drawPath(const VPath &path, const VBrush &brush)
{
    if (!mQPainter || path.empty()) return;

    QPainterPath qPath = convertVPathToQPainterPath(path);
    if (qPath.isEmpty()) return;

    setupQPainter(brush);

    // 填充绘制
    mQPainter->fillPath(qPath, mQPainter->brush());
}

void VPainterQt::drawPath(const VPath &path, const VBrush &brush, CapStyle cap, JoinStyle join, float width)
{
    if (!mQPainter || path.empty()) return;

    QPainterPath qPath = convertVPathToQPainterPath(path);

    // 设置描边参数
    QPen pen;
    pen.setColor(QColor(255, 255, 255)); // 默认白色，会被brush覆盖
    pen.setWidthF(width);

    // 转换线帽样式
    switch (cap) {
        case CapStyle::Round:
            pen.setCapStyle(Qt::RoundCap);
            break;
        case CapStyle::Square:
            pen.setCapStyle(Qt::SquareCap);
            break;
        default:
            pen.setCapStyle(Qt::FlatCap);
            break;
    }

    // 转换连接样式
    switch (join) {
        case JoinStyle::Round:
            pen.setJoinStyle(Qt::RoundJoin);
            break;
        case JoinStyle::Bevel:
            pen.setJoinStyle(Qt::BevelJoin);
            break;
        default:
            pen.setJoinStyle(Qt::MiterJoin);
            break;
    }

    // 设置画刷颜色到画笔
    if (brush.type() == VBrush::Type::Solid) {
        VColor color = brush.mColor;
        pen.setColor(QColor(color.r, color.g, color.b, color.a));
    }

    mQPainter->setPen(pen);
    mQPainter->drawPath(qPath);
}

QPainterPath VPainterQt::convertVPathToQPainterPath(const VPath &path)
{
    QPainterPath qPath;

    if (path.empty()) return qPath;

    const auto &elements = path.elements();
    const auto &points = path.points();

    if (elements.empty() || points.empty()) return qPath;

    size_t pointIndex = 0;
    for (VPath::Element element : elements) {
        switch (element) {
            case VPath::Element::MoveTo:
                if (pointIndex < points.size()) {
                    qPath.moveTo(points[pointIndex].x(), points[pointIndex].y());
                    pointIndex++;
                }
                break;

            case VPath::Element::LineTo:
                if (pointIndex < points.size()) {
                    qPath.lineTo(points[pointIndex].x(), points[pointIndex].y());
                    pointIndex++;
                }
                break;

            case VPath::Element::CubicTo:
                if (pointIndex + 2 < points.size()) {
                    qPath.cubicTo(
                        points[pointIndex].x(), points[pointIndex].y(),
                        points[pointIndex + 1].x(), points[pointIndex + 1].y(),
                        points[pointIndex + 2].x(), points[pointIndex + 2].y()
                    );
                    pointIndex += 3;
                }
                break;

            case VPath::Element::Close:
                qPath.closeSubpath();
                break;
        }
    }

    return qPath;
}

void VPainterQt::setupQPainter(const VBrush &brush)
{
    if (!mQPainter) return;

    switch (brush.type()) {
        case VBrush::Type::Solid: {
            VColor color = brush.mColor;
            QColor qColor(color.r, color.g, color.b, color.a);
            mQPainter->setBrush(QBrush(qColor));
            break;
        }
        case VBrush::Type::LinearGradient: {
            // TODO: 实现线性渐变
            mQPainter->setBrush(QBrush(Qt::red));
            break;
        }
        case VBrush::Type::RadialGradient: {
            // TODO: 实现径向渐变
            mQPainter->setBrush(QBrush(Qt::blue));
            break;
        }
        default:
            mQPainter->setBrush(QBrush(Qt::NoBrush));
            break;
    }
}

V_END_NAMESPACE

#endif // LOTTIE_QT