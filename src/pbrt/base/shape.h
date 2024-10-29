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
/*
    定义了物体形状的几何特征
*/
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
        求交，返回最近的交点的信息
    */
    PBRT_CPU_GPU inline pstd::optional<ShapeIntersection> Intersect(
        const Ray &ray, Float tMax = Infinity) const;

    /*
        检测是否相交，比Intersect()开销小，经常用于阴影光线计算中，
        用来测试场景中从某点是否能看到光源。若此方法返回true，相交的范围参数就用以
        参数hitt0和hitt1返回。在(0,tMax)之外的相交会被忽略。
        若光线原点在盒子中，会在hitt0返回0。
    */
    PBRT_CPU_GPU inline bool IntersectP(const Ray &ray, Float tMax = Infinity) const;

    /*
        用于面光源返回其在渲染空间坐标系下的面积
    */
    PBRT_CPU_GPU inline Float Area() const;

    /*
        在表面上某点采样，把形状某点作为发光点时会用到
    */
    PBRT_CPU_GPU inline pstd::optional<ShapeSample> Sample(Point2f u) const;

    /*
        当使用多重重要性采样时,会使用其他采样方式来生成样本,会用到此方法
    */
    PBRT_CPU_GPU inline Float PDF(const Interaction &) const;

    /*
        利用概率密度来生成一个关于参考点立体角的点，计算面积光采样时会用到，因为
        面积光采样的计算，会把直接光照的积分看作一个从参考点方向上的积分，用这个
        点的立体角来表达这些采样密度会更方便
    */
    PBRT_CPU_GPU inline pstd::optional<ShapeSample> Sample(const ShapeSampleContext &ctx,
                                                           Point2f u) const;

    PBRT_CPU_GPU inline Float PDF(const ShapeSampleContext &ctx, Vector3f wi) const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_SHAPE_H
