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

    // 每个像素点要取几个采样
    PBRT_CPU_GPU inline int SamplesPerPixel() const;

    /*
        当某个积分器已经准备用给出的像素样本运行时，会从StartPixelSample()开始，
        这个函数提供了图像中像素点的坐标和像素点的样本的索引(索引值大于等于0，
        且小于SamplePerPixel()的值)。积分器可能也会提供一个起始维度，指定采样生成
        应该从哪个维度开始
    */
    PBRT_CPU_GPU inline void StartPixelSample(Point2i p, int sampleIndex,
                                              int dimension = 0);
    /*
        积分器可以通过Get1D()和Get2D()同时请求d维的一个或多个样本点的维度值。
        虽然一个二维的样本值可以通过调用2次Get1D()获取，但是一些采样器在知道有
        2个维度值要生成的时候，能返回更好的样本值。然而，接口不支持直接返回3维
        或更高维度的样本值，因为一般来说，渲染算法的实现不需要。若有这种场景，
        可以通过调用低维度的样本值生成方法来构建高维度的样本点。
    */
    PBRT_CPU_GPU inline Float Get1D();
    PBRT_CPU_GPU inline Point2f Get2D();

    /*
        GetPixel2D()被用来获取二维的样本，用来确定被采样的点对应胶片平面上的
        哪个点。某些采样器的实现，在处理样本的胶片采样点的维度值的方式和处理在
        其他维度下的二维样本值的方式不同
    */
    PBRT_CPU_GPU inline Point2f GetPixel2D();

    /*
        如果是多线程，单个Sampler被并行访问是不安全的。因此，积分器调用Clone()
        来获取初始Sampler的拷贝，这样每个线程有自己的一份
    */
    Sampler Clone(Allocator alloc = {});

    std::string ToString() const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_SAMPLER_H
