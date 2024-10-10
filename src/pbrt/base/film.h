// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_BASE_FILM_H
#define PBRT_BASE_FILM_H

#include <pbrt/pbrt.h>

#include <pbrt/base/filter.h>
#include <pbrt/util/pstd.h>
#include <pbrt/util/taggedptr.h>

#include <string>

namespace pbrt {

class VisibleSurface;
class RGBFilm;
class GBufferFilm;
class SpectralFilm;
class PixelSensor;

// Film Definition
class Film : public TaggedPointer<RGBFilm, GBufferFilm, SpectralFilm> {
  public:
    // Film Interface
    /*
        样本需要被提供给胶片，有两种方式
        第一种是在积分器估计辐射度时，让Sampler从胶片上选择一个点，通过此函数提供给Film对象
        要提供几个参数：
        1. 样本的像素点坐标pFilm
        2. 样本的光谱辐射度L
        3. 样本的波长lambda
        4. 可选，沿着样本的相机光线碰到的第一个几何体VisibleSurface
        5. 计算过滤器采样时的样本权重weight
    */
    PBRT_CPU_GPU inline void AddSample(Point2i pFilm, SampledSpectrum L,
                                       const SampledWavelengths &lambda,
                                       const VisibleSurface *visibleSurface,
                                       Float weight);

    /*
        返回所有可能生成的样本的边框,注意，这个边框跟图像像素点的边框不一样，
        一般来说，像素滤波器范围比单个像素要更宽
    */
    PBRT_CPU_GPU inline Bounds2f SampleBounds() const;

    /*
        检测是否传了*VisibleSurface到addSample()里，若VisibleSurface没传，
        这个函数能允许积分器避免初始化VisibleSurface的昂贵开销
    */
    PBRT_CPU_GPU
    bool UsesVisibleSurface() const;

    /*
        某些从光源开始追踪路径的算法(比如双向路径追踪)，需要把光贡献量splat到任意像素
        此函数把某个值溅射到图像中的某个位置
        由于可能被多个线程并行调用，函数的实现必须考虑实现互斥或原子操作
    */
    PBRT_CPU_GPU
    void AddSplat(Point2f p, SampledSpectrum v, const SampledWavelengths &lambda);

    /*
        Film的实现必须提供SampleWavelengths()函数，来从胶片感光元件能响应的波段采样
    */
    PBRT_CPU_GPU inline SampledWavelengths SampleWavelengths(Float u) const;

    /*
        一些方便获取图像信息的函数，分辨率，图像范围，对角线长度，单位为米
    */
    PBRT_CPU_GPU inline Point2i FullResolution() const;
    PBRT_CPU_GPU inline Bounds2i PixelBounds() const;
    PBRT_CPU_GPU inline Float Diagonal() const;

    void WriteImage(ImageMetadata metadata, Float splatScale = 1);

    PBRT_CPU_GPU inline RGB ToOutputRGB(SampledSpectrum L,
                                        const SampledWavelengths &lambda) const;

    Image GetImage(ImageMetadata *metadata, Float splatScale = 1);

    PBRT_CPU_GPU
    RGB GetPixelRGB(Point2i p, Float splatScale = 1) const;

    PBRT_CPU_GPU inline Filter GetFilter() const;
    PBRT_CPU_GPU inline const PixelSensor *GetPixelSensor() const;
    std::string GetFilename() const;

    using TaggedPointer::TaggedPointer;

    static Film Create(const std::string &name, const ParameterDictionary &parameters,
                       Float exposureTime, const CameraTransform &cameraTransform,
                       Filter filter, const FileLoc *loc, Allocator alloc);

    std::string ToString() const;

    PBRT_CPU_GPU inline void ResetPixel(Point2i p);
};

}  // namespace pbrt

#endif  // PBRT_BASE_FILM_H
