// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_BASE_LIGHT_H
#define PBRT_BASE_LIGHT_H

#include <pbrt/pbrt.h>

#include <pbrt/base/medium.h>
#include <pbrt/base/shape.h>
#include <pbrt/base/texture.h>
#include <pbrt/util/pstd.h>
#include <pbrt/util/taggedptr.h>

#include <string>

namespace pbrt {

// LightType Definition
enum class LightType { DeltaPosition, DeltaDirection, Area, Infinite };

class PointLight;
class DistantLight;
class ProjectionLight;
class GoniometricLight;
class DiffuseAreaLight;
class UniformInfiniteLight;
class ImageInfiniteLight;
class PortalImageInfiniteLight;
class SpotLight;

class LightSampleContext;
class LightBounds;
class CompactLightBounds;
struct LightLiSample;
struct LightLeSample;

// Light Definition
/*
    所有光源必须实现此接口
*/
class Light : public TaggedPointer<  // Light Source Types
                  PointLight, DistantLight, ProjectionLight, GoniometricLight, SpotLight,
                  DiffuseAreaLight, UniformInfiniteLight, ImageInfiniteLight,
                  PortalImageInfiniteLight

                  > {
  public:
    // Light Interface
    using TaggedPointer::TaggedPointer;

    static Light Create(const std::string &name, const ParameterDictionary &parameters,
                        const Transform &renderFromLight,
                        const CameraTransform &cameraTransform, Medium outsideMedium,
                        const FileLoc *loc, Allocator alloc);
    static Light CreateArea(const std::string &name,
                            const ParameterDictionary &parameters,
                            const Transform &renderFromLight,
                            const MediumInterface &mediumInterface, const Shape shape,
                            FloatTexture alpha, const FileLoc *loc, Allocator alloc);

    // 核心接口，返回光源发出的光辐射功率Phi
    SampledSpectrum Phi(SampledWavelengths lambda) const;

    /*
        Light接口并没有对所有类型的光源做抽象，为了效率和正确性，pbrt中的积分器有时需要根据
        不同类型的光源做不同的处理
        type分为四种类型:
        1. DeltaPosition: delta即狄拉克delta分布，也就是空间中的点光源
        2. DeltaDirection: 光只在某个方向发出
        3. Area: 光从一个几何表面发出
        4. Infinite: 光没有几何形状，在无限远处
    */
    PBRT_CPU_GPU inline LightType Type() const;

    // 核心接口:根据参考点返回这个点接收到的辐射量等一系列有用的参数
    PBRT_CPU_GPU inline pstd::optional<LightLiSample> SampleLi(
        LightSampleContext ctx, Point2f u, SampledWavelengths lambda,
        bool allowIncompletePDF = false) const;

    // 在多重重要性采样时有用
    PBRT_CPU_GPU inline Float PDF_Li(LightSampleContext ctx, Vector3f wi,
                                     bool allowIncompletePDF = false) const;

    std::string ToString() const;

    // AreaLights only
    // 面光源有用
    PBRT_CPU_GPU inline SampledSpectrum L(Point3f p, Normal3f n, Point2f uv, Vector3f w,
                                          const SampledWavelengths &lambda) const;

    // InfiniteLights only
    // 无限远光源有用
    PBRT_CPU_GPU inline SampledSpectrum Le(const Ray &ray,
                                           const SampledWavelengths &lambda) const;

    // 一些光源需要知道场景的空间边界时有用
    void Preprocess(const Bounds3f &sceneBounds);

    pstd::optional<LightBounds> Bounds() const;

    PBRT_CPU_GPU
    pstd::optional<LightLeSample> SampleLe(Point2f u1, Point2f u2,
                                           SampledWavelengths &lambda, Float time) const;

    PBRT_CPU_GPU
    void PDF_Le(const Ray &ray, Float *pdfPos, Float *pdfDir) const;

    // AreaLights only
    PBRT_CPU_GPU
    void PDF_Le(const Interaction &intr, Vector3f w, Float *pdfPos, Float *pdfDir) const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_LIGHT_H
