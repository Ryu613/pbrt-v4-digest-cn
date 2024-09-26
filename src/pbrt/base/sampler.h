// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_BASE_SAMPLER_H
#define PBRT_BASE_SAMPLER_H

#include <pbrt/pbrt.h>

#include <pbrt/util/taggedptr.h>
#include <pbrt/util/vecmath.h>

#include <string>
#include <vector>

namespace pbrt {

/*
    包含了相机光线需要的所有样本值
    pFilm: 这条光线在胶片上的对应点的位置，这个点带有光辐射的量
    pLens: 这条光线通过镜头时的点的位置
    time: 光线采样的时间点，若镜头会移动，则代表当生成光线时，相机所在的位置
    filterWeight: 这条光线的辐射量的缩放因子，是为了重建过滤器为每个像素点过滤图像样本时使用

*/
struct CameraSample {
    Point2f pFilm;
    Point2f pLens;
    Float time = 0;
    Float filterWeight = 1;
    std::string ToString() const;
};

// Sampler Declarations
class HaltonSampler;
class PaddedSobolSampler;
class PMJ02BNSampler;
class IndependentSampler;
class SobolSampler;
class StratifiedSampler;
class ZSobolSampler;
class MLTSampler;
class DebugMLTSampler;

// Sampler Definition
class Sampler
    : public TaggedPointer<  // Sampler Types
          PMJ02BNSampler, IndependentSampler, StratifiedSampler, HaltonSampler,
          PaddedSobolSampler, SobolSampler, ZSobolSampler, MLTSampler, DebugMLTSampler

          > {
  public:
    // Sampler Interface
    using TaggedPointer::TaggedPointer;

    static Sampler Create(const std::string &name, const ParameterDictionary &parameters,
                          Point2i fullResolution, const FileLoc *loc, Allocator alloc);

    PBRT_CPU_GPU inline int SamplesPerPixel() const;

    PBRT_CPU_GPU inline void StartPixelSample(Point2i p, int sampleIndex,
                                              int dimension = 0);

    PBRT_CPU_GPU inline Float Get1D();
    PBRT_CPU_GPU inline Point2f Get2D();

    PBRT_CPU_GPU inline Point2f GetPixel2D();

    Sampler Clone(Allocator alloc = {});

    std::string ToString() const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_SAMPLER_H
