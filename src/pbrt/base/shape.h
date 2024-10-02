// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_BASE_SHAPE_H
#define PBRT_BASE_SHAPE_H

#include <pbrt/pbrt.h>

#include <pbrt/base/texture.h>
#include <pbrt/util/taggedptr.h>
#include <pbrt/util/vecmath.h>

#include <map>
#include <string>

namespace pbrt {

// Shape Declarations
class Triangle;
class BilinearPatch;
class Curve;
class Sphere;
class Cylinder;
class Disk;

struct ShapeSample;
struct ShapeIntersection;
struct ShapeSampleContext;

// Shape Definition
class Shape
    : public TaggedPointer<Sphere, Cylinder, Disk, Triangle, BilinearPatch, Curve> {
  public:
    // Shape Interface
    using TaggedPointer::TaggedPointer;

    static pstd::vector<Shape> Create(
        const std::string &name, const Transform *renderFromObject,
        const Transform *objectFromRender, bool reverseOrientation,
        const ParameterDictionary &parameters,
        const std::map<std::string, FloatTexture> &floatTextures, const FileLoc *loc,
        Allocator alloc);
    std::string ToString() const;

    /*
        每个Shape实现都必须能包围它本身，用Bounds3f来封装，返回的包围盒需要是渲染坐标系下的坐标
    */
    PBRT_CPU_GPU inline Bounds3f Bounds() const;

    /*
        把这个Shape表面的法线包围其中，用于在光线计算中，检测某点是否能被发光体照到
    */
    PBRT_CPU_GPU inline DirectionCone NormalBounds() const;

    /*
        Shape的实现必须提供2个函数
    */
    PBRT_CPU_GPU inline pstd::optional<ShapeIntersection> Intersect(
        const Ray &ray, Float tMax = Infinity) const;

    PBRT_CPU_GPU inline bool IntersectP(const Ray &ray, Float tMax = Infinity) const;

    PBRT_CPU_GPU inline Float Area() const;

    PBRT_CPU_GPU inline pstd::optional<ShapeSample> Sample(Point2f u) const;

    PBRT_CPU_GPU inline Float PDF(const Interaction &) const;

    PBRT_CPU_GPU inline pstd::optional<ShapeSample> Sample(const ShapeSampleContext &ctx,
                                                           Point2f u) const;

    PBRT_CPU_GPU inline Float PDF(const ShapeSampleContext &ctx, Vector3f wi) const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_SHAPE_H
