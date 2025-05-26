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

#include "vdrawable.h"
#include "vdasher.h"
#include "vraster.h"
#include "vpainter.h"

VDrawable::VDrawable(VDrawable::Type type)
{
    setType(type);
}

VDrawable::~VDrawable() noexcept
{
    if (mStrokeInfo) {
        if (mType == Type::StrokeWithDash) {
            delete static_cast<StrokeWithDashInfo *>(mStrokeInfo);
        } else {
            delete mStrokeInfo;
        }
    }
}

void VDrawable::setType(VDrawable::Type type)
{
    mType = type;
    if (mType == VDrawable::Type::Stroke) {
        mStrokeInfo = new StrokeInfo();
    } else if (mType == VDrawable::Type::StrokeWithDash) {
        mStrokeInfo = new StrokeWithDashInfo();
    }
}

void VDrawable::applyDashOp()
{
    if (mStrokeInfo && (mType == Type::StrokeWithDash)) {
        auto obj = static_cast<StrokeWithDashInfo *>(mStrokeInfo);
        if (!obj->mDash.empty()) {
            VDasher dasher(obj->mDash.data(), obj->mDash.size());
            mPath.clone(dasher.dashed(mPath));
        }
    }
}

void VDrawable::preprocess(const VRect &clip)
{
    if (mFlag & (DirtyState::Path)) {
        // 保存原始路径副本，用于矢量渲染
        mOriginalPath = mPath;
        
        if (mType == Type::Fill) {
            mRasterizer.rasterize(std::move(mPath), mFillRule, clip);
        } else {
            applyDashOp();
            mRasterizer.rasterize(std::move(mPath), mStrokeInfo->cap, mStrokeInfo->join,
                                  mStrokeInfo->width, mStrokeInfo->miterLimit, clip);
        }
        mPath = {};
        mFlag &= ~DirtyFlag(DirtyState::Path);
    }
}

VRle VDrawable::rle()
{
    return mRasterizer.rle();
}

// 添加直接绘制VPath的方法，用于矢量渲染器
void VDrawable::drawPath(VPainter *painter)
{
    // 使用原始路径进行矢量渲染
    VPath pathToUse = mOriginalPath.empty() ? mPath : mOriginalPath;
    
    if (pathToUse.empty()) return;
    
    // 应用虚线操作（如果需要）
    VPath finalPath = pathToUse;
    if (mStrokeInfo && (mType == Type::StrokeWithDash)) {
        auto obj = static_cast<StrokeWithDashInfo *>(mStrokeInfo);
        if (!obj->mDash.empty()) {
            VDasher dasher(obj->mDash.data(), obj->mDash.size());
            finalPath = dasher.dashed(pathToUse);
        }
    }
    
    // 根据类型绘制
    if (mType == Type::Fill) {
        painter->drawPath(finalPath, mBrush);
    } else {
        // 描边绘制
        painter->drawPath(finalPath, mBrush, mStrokeInfo->cap, mStrokeInfo->join, mStrokeInfo->width);
    }
}

void VDrawable::setStrokeInfo(CapStyle cap, JoinStyle join, float miterLimit,
                              float strokeWidth)
{
    assert(mStrokeInfo);
    if ((mStrokeInfo->cap == cap) && (mStrokeInfo->join == join) &&
        vCompare(mStrokeInfo->miterLimit, miterLimit) &&
        vCompare(mStrokeInfo->width, strokeWidth))
        return;

    mStrokeInfo->cap = cap;
    mStrokeInfo->join = join;
    mStrokeInfo->miterLimit = miterLimit;
    mStrokeInfo->width = strokeWidth;
    mFlag |= DirtyState::Path;
}

void VDrawable::setDashInfo(std::vector<float> &dashInfo)
{
    assert(mStrokeInfo);
    assert(mType == VDrawable::Type::StrokeWithDash);

    auto obj = static_cast<StrokeWithDashInfo *>(mStrokeInfo);
    bool hasChanged = false;

    if (obj->mDash.size() == dashInfo.size()) {
        for (uint32_t i = 0; i < dashInfo.size(); ++i) {
            if (!vCompare(obj->mDash[i], dashInfo[i])) {
                hasChanged = true;
                break;
            }
        }
    } else {
        hasChanged = true;
    }

    if (!hasChanged) return;

    obj->mDash = dashInfo;

    mFlag |= DirtyState::Path;
}

void VDrawable::setPath(const VPath &path)
{
    mPath = path;
    mFlag |= DirtyState::Path;
}
