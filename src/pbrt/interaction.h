// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_INTERACTION_H
#define PBRT_INTERACTION_H

#include <pbrt/pbrt.h>

#include <pbrt/base/bssrdf.h>
#include <pbrt/base/camera.h>
#include <pbrt/base/light.h>
#include <pbrt/base/material.h>
#include <pbrt/base/medium.h>
#include <pbrt/base/sampler.h>
#include <pbrt/ray.h>
#include <pbrt/util/spectrum.h>
#include <pbrt/util/taggedptr.h>
#include <pbrt/util/vecmath.h>

#include <limits>

namespace pbrt {

// Interaction Definition
/*
    光(在介质种)与物体表面相互作用，也可用于表示介质与介质之间的相互作用
*/
class Interaction {
  public:
    // Interaction Public Methods
    Interaction() = default;

    PBRT_CPU_GPU
    Interaction(Point3fi pi, Normal3f n, Point2f uv, Vector3f wo, Float time)
        : pi(pi), n(n), uv(uv), wo(Normalize(wo)), time(time) {}

    PBRT_CPU_GPU
    Point3f p() const { return Point3f(pi); }

    /*
        查某个Interaction的指针或引用是否是两个子类之一, 根据法线是否为0判断
    */
    PBRT_CPU_GPU
    bool IsSurfaceInteraction() const { return n != Normal3f(0, 0, 0); }
    PBRT_CPU_GPU
    bool IsMediumInteraction() const { return !IsSurfaceInteraction(); }

    /*
        检查父类转换为子类是否可行
    */
    PBRT_CPU_GPU
    const SurfaceInteraction &AsSurface() const {
        CHECK(IsSurfaceInteraction());
        return (const SurfaceInteraction &)*this;
    }

    PBRT_CPU_GPU
    SurfaceInteraction &AsSurface() {
        CHECK(IsSurfaceInteraction());
        return (SurfaceInteraction &)*this;
    }

    // used by medium ctor
    PBRT_CPU_GPU
    Interaction(Point3f p, Vector3f wo, Float time, Medium medium)
        : pi(p), time(time), wo(wo), medium(medium) {}
    PBRT_CPU_GPU
    Interaction(Point3f p, Normal3f n, Float time, Medium medium)
        : pi(p), n(n), time(time), medium(medium) {}
    PBRT_CPU_GPU
    Interaction(Point3f p, Point2f uv) : pi(p), uv(uv) {}
    PBRT_CPU_GPU
    Interaction(const Point3fi &pi, Normal3f n, Float time = 0, Point2f uv = {})
        : pi(pi), n(n), uv(uv), time(time) {}
    PBRT_CPU_GPU
    Interaction(const Point3fi &pi, Normal3f n, Point2f uv) : pi(pi), n(n), uv(uv) {}
    PBRT_CPU_GPU
    Interaction(Point3f p, Float time, Medium medium)
        : pi(p), time(time), medium(medium) {}
    PBRT_CPU_GPU
    Interaction(Point3f p, const MediumInterface *mediumInterface)
        : pi(p), mediumInterface(mediumInterface) {}
    PBRT_CPU_GPU
    Interaction(Point3f p, Float time, const MediumInterface *mediumInterface)
        : pi(p), time(time), mediumInterface(mediumInterface) {}
    PBRT_CPU_GPU
    const MediumInteraction &AsMedium() const {
        CHECK(IsMediumInteraction());
        return (const MediumInteraction &)*this;
    }
    PBRT_CPU_GPU
    MediumInteraction &AsMedium() {
        CHECK(IsMediumInteraction());
        return (MediumInteraction &)*this;
    }

    std::string ToString() const;

    PBRT_CPU_GPU
    Point3f OffsetRayOrigin(Vector3f w) const { return pbrt::OffsetRayOrigin(pi, n, w); }

    PBRT_CPU_GPU
    Point3f OffsetRayOrigin(Point3f pt) const { return OffsetRayOrigin(pt - p()); }

    PBRT_CPU_GPU
    RayDifferential SpawnRay(Vector3f d) const {
        return RayDifferential(OffsetRayOrigin(d), d, time, GetMedium(d));
    }

    PBRT_CPU_GPU
    Ray SpawnRayTo(Point3f p2) const {
        Ray r = pbrt::SpawnRayTo(pi, n, time, p2);
        r.medium = GetMedium(r.d);
        return r;
    }

    PBRT_CPU_GPU
    Ray SpawnRayTo(const Interaction &it) const {
        Ray r = pbrt::SpawnRayTo(pi, n, time, it.pi, it.n);
        r.medium = GetMedium(r.d);
        return r;
    }

    PBRT_CPU_GPU
    Medium GetMedium(Vector3f w) const {
        if (mediumInterface)
            return Dot(w, n) > 0 ? mediumInterface->outside : mediumInterface->inside;
        return medium;
    }

    PBRT_CPU_GPU
    Medium GetMedium() const {
        if (mediumInterface)
            DCHECK_EQ(mediumInterface->inside, mediumInterface->outside);
        return mediumInterface ? mediumInterface->inside : medium;
    }

    // Interaction Public Members
    // pi是一个域， 表示交点处的附近的区域，用于避免光线在离开表面时不正确的自相交问题
    Point3fi pi;
    // 于设置离开交互点时生成的光线的时间点
    Float time = 0;
    // 出射光反向向量，为什么是反向的？因为光是从相机射出，不是从光源射出
    Vector3f wo;
    // 交互点对应的法线和uv，不放在SurfaceInteraction是为了方便调用
    Normal3f n;
    Point2f uv;
    /*
        当Interaction用作表示两种介质之间的相互作用时使用
    */
    const MediumInterface *mediumInterface = nullptr;
    /*
        当Interaction用作表示在某种散射介质中的点的属性时使用
    */
    Medium medium = nullptr;
};

// MediumInteraction Definition
class MediumInteraction : public Interaction {
  public:
    // MediumInteraction Public Methods
    PBRT_CPU_GPU
    MediumInteraction() : phase(nullptr) {}
    PBRT_CPU_GPU
    MediumInteraction(Point3f p, Vector3f wo, Float time, Medium medium,
                      PhaseFunction phase)
        : Interaction(p, wo, time, medium), phase(phase) {}

    std::string ToString() const;

    // MediumInteraction Public Members
    PhaseFunction phase;
};

// SurfaceInteraction Definition
/*
    交点对应表面的一些几何相关的量
*/
class SurfaceInteraction : public Interaction {
  public:
    // SurfaceInteraction Public Methods
    SurfaceInteraction() = default;

    PBRT_CPU_GPU
    SurfaceInteraction(Point3fi pi, Point2f uv, Vector3f wo, Vector3f dpdu, Vector3f dpdv,
                       Normal3f dndu, Normal3f dndv, Float time, bool flipNormal)
        : Interaction(pi, Normal3f(Normalize(Cross(dpdu, dpdv))), uv, wo, time),
          dpdu(dpdu),
          dpdv(dpdv),
          dndu(dndu),
          dndv(dndv) {
        // Initialize shading geometry from true geometry
        shading.n = n;
        shading.dpdu = dpdu;
        shading.dpdv = dpdv;
        shading.dndu = dndu;
        shading.dndv = dndv;

        // Adjust normal based on orientation and handedness
        if (flipNormal) {
            n *= -1;
            shading.n *= -1;
        }
    }

    PBRT_CPU_GPU
    SurfaceInteraction(Point3fi pi, Point2f uv, Vector3f wo, Vector3f dpdu, Vector3f dpdv,
                       Normal3f dndu, Normal3f dndv, Float time, bool flipNormal,
                       int faceIndex)
        : SurfaceInteraction(pi, uv, wo, dpdu, dpdv, dndu, dndv, time, flipNormal) {
        this->faceIndex = faceIndex;
    }

    PBRT_CPU_GPU
    void SetShadingGeometry(Normal3f ns, Vector3f dpdus, Vector3f dpdvs, Normal3f dndus,
                            Normal3f dndvs, bool orientationIsAuthoritative) {
        // Compute _shading.n_ for _SurfaceInteraction_
        shading.n = ns;
        DCHECK_NE(shading.n, Normal3f(0, 0, 0));
        if (orientationIsAuthoritative)
            n = FaceForward(n, shading.n);
        else
            shading.n = FaceForward(shading.n, n);

        // Initialize _shading_ partial derivative values
        shading.dpdu = dpdus;
        shading.dpdv = dpdvs;
        shading.dndu = dndus;
        shading.dndv = dndvs;
        while (LengthSquared(shading.dpdu) > 1e16f ||
               LengthSquared(shading.dpdv) > 1e16f) {
            shading.dpdu /= 1e8f;
            shading.dpdv /= 1e8f;
        }
    }

    std::string ToString() const;

    void SetIntersectionProperties(Material mtl, Light area,
                                   const MediumInterface *primMediumInterface,
                                   Medium rayMedium) {
        material = mtl;
        areaLight = area;
        CHECK_GE(Dot(n, shading.n), 0.);
        // Set medium properties at surface intersection
        if (primMediumInterface && primMediumInterface->IsMediumTransition())
            mediumInterface = primMediumInterface;
        else
            medium = rayMedium;
    }

    PBRT_CPU_GPU
    void ComputeDifferentials(const RayDifferential &r, Camera camera,
                              int samplesPerPixel);

    PBRT_CPU_GPU
    void SkipIntersection(RayDifferential *ray, Float t) const;

    using Interaction::SpawnRay;
    PBRT_CPU_GPU
    RayDifferential SpawnRay(const RayDifferential &rayi, const BSDF &bsdf, Vector3f wi,
                             int /*BxDFFlags*/ flags, Float eta) const;

    BSDF GetBSDF(const RayDifferential &ray, SampledWavelengths &lambda, Camera camera,
                 ScratchBuffer &scratchBuffer, Sampler sampler);
    BSSRDF GetBSSRDF(const RayDifferential &ray, SampledWavelengths &lambda,
                     Camera camera, ScratchBuffer &scratchBuffer);

    PBRT_CPU_GPU
    SampledSpectrum Le(Vector3f w, const SampledWavelengths &lambda) const;

    // SurfaceInteraction Public Members
    // 点p在uv方向的偏导数
    Vector3f dpdu, dpdv;
    // 法线的uv偏导数
    Normal3f dndu, dndv;
    // 用于支持凹凸贴图或网格的每个顶点插值的法线，把原始坐标和法线处理以后的值，着色用
    struct {
        Normal3f n;
        Vector3f dpdu, dpdv;
        Normal3f dndu, dndv;
    } shading;
    int faceIndex = 0;
    Material material;
    Light areaLight;
    Vector3f dpdx, dpdy;
    Float dudx = 0, dvdx = 0, dudy = 0, dvdy = 0;
};

}  // namespace pbrt

#endif  // PBRT_INTERACTION_H
