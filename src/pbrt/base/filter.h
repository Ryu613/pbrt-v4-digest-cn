// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_BASE_FILTER_H
#define PBRT_BASE_FILTER_H

#include <pbrt/pbrt.h>

#include <pbrt/util/taggedptr.h>

#include <string>

namespace pbrt {

// Filter Declarations
struct FilterSample;
class BoxFilter;
class GaussianFilter;
class MitchellFilter;
class LanczosSincFilter;
class TriangleFilter;

// Filter Definition
class Filter : public TaggedPointer<BoxFilter, GaussianFilter, MitchellFilter,
                                    LanczosSincFilter, TriangleFilter> {
  public:
    // Filter Interface
    using TaggedPointer::TaggedPointer;

    static Filter Create(const std::string &name, const ParameterDictionary &parameters,
                         const FileLoc *loc, Allocator alloc);

    /*
        滤波器范围根据每个像素点从原点到剪切点的半径来定
        滤波器支持的范围是总体非零的区域，等于半径的两倍
    */
    PBRT_CPU_GPU inline Vector2f Radius() const;

    /*
        计算他们的滤波函数，若被范围外的点调用，返回0
    */
    PBRT_CPU_GPU inline Float Evaluate(Point2f p) const;

    PBRT_CPU_GPU inline Float Integral() const;

    /*
        重要性采样方法，取一个二维随机点u，值域在[0,1)
    */
    PBRT_CPU_GPU inline FilterSample Sample(Point2f u) const;

    std::string ToString() const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_FILTER_H
