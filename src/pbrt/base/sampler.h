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
    filterWeight: 这条光线的辐射量的缩放因子，是为了重建滤波器为每个像素点过滤图像样本时使用

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
/*
    采样器的任务是生成均匀的d-维的样本点，每个点的坐标值都在[0,1)间。
    点的维度总数没有提前设定，采样器必须根据光传输算法执行时的计算要求，生成额外的维度
    使用样本值的代码必须谨慎编写，保证样本每个维度都被使用，且顺序一致
*/
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

    // 每个像素点要取的样本数
    PBRT_CPU_GPU inline int SamplesPerPixel() const;

    /*
        积分器开始采样时调用，最后一个参数用于指定从哪个维度开始生成
        调用原因有二:
        1. 有些采样器会利用之前的采样来调整后续采样，以便优化采样的整体分布情况
        2. 使采样状态可确定，这样的话每次生成的采样都是与前次一致的，方便调试
    */
    PBRT_CPU_GPU inline void StartPixelSample(Point2i p, int sampleIndex,
                                              int dimension = 0);
    /*
        积分器可以通过Get1D()和Get2D()一次性请求d维样本点的1个或2个维度。
        虽然二维的样本值可以通过调用2次Get1D()获取，
        但是一些采样器如果知道有2个维度值会被一起使用时，能生成更好的样本值
    */
    PBRT_CPU_GPU inline Float Get1D();
    PBRT_CPU_GPU inline Point2f Get2D();

    /*
        拿到一个二维样本，用来确定在胶片平面上被采样的点
        有些采样器是自己实现，另一些是直接调用Get2D()实现的
    */
    PBRT_CPU_GPU inline Point2f GetPixel2D();

    /*
        单个Sampler被多线程并行访问是不安全的
        积分器调用Clone()为每个线程获取一份初始Sampler的拷贝
    */
    Sampler Clone(Allocator alloc = {});

    std::string ToString() const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_SAMPLER_H
