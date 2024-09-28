// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_BASE_CAMERA_H
#define PBRT_BASE_CAMERA_H

#include <pbrt/pbrt.h>

#include <pbrt/base/film.h>
#include <pbrt/base/filter.h>
#include <pbrt/util/taggedptr.h>
#include <pbrt/util/transform.h>
#include <pbrt/util/vecmath.h>

#include <string>

namespace pbrt {

// Camera Declarations
struct CameraRay;
struct CameraRayDifferential;
struct CameraWiSample;

struct CameraSample;
class CameraTransform;

class PerspectiveCamera;
class OrthographicCamera;
class SphericalCamera;
class RealisticCamera;

/*
    相机概念的顶层抽象
*/
class Camera : public TaggedPointer<PerspectiveCamera, OrthographicCamera,
                                    SphericalCamera, RealisticCamera> {
  public:
    // Camera Interface
    using TaggedPointer::TaggedPointer;

    static Camera Create(const std::string &name, const ParameterDictionary &parameters,
                         Medium medium, const CameraTransform &cameraTransform, Film film,
                         const FileLoc *loc, Allocator alloc);

    std::string ToString() const;

    /*
        必须实现的方法，用于对应图像采样的光线的计算，返回的光线需要归一化
        若给定的CameraSample对象由于一些原因没有有效的光线，那么pstd::optional
        中的返回值需要被重置。

        传入进来的SampleWavelengths(样本波长)不是常量引用，故相机就可以模拟镜头
        的色散效果，在这种情况下，光线只追踪单一波长的光，并且 GenerateRay() 
        方法将调用 SampledWavelengths::TerminateSecondary()

        传入到此函数的CameraSample结构体，包含了相机光线需要的所有样本值。
    */
    PBRT_CPU_GPU inline pstd::optional<CameraRay> GenerateRay(
        CameraSample sample, SampledWavelengths &lambda) const;

    /*
        相机必须提供此方法的实现，这个方法不仅会类似GenerateRay()计算主光线，
        还会计算位于胶片沿x,y方向上移动一个像素的微分光线，
        用来代表特定相机光线的采样所对应的胶片的区域大小，
        用于计算抗锯齿时的纹理查找，提高图像质量
    */
    PBRT_CPU_GPU
    pstd::optional<CameraRayDifferential> GenerateRayDifferential(
        CameraSample sample, SampledWavelengths &lambda) const;

    /*
        相机的实现必须提供对应Film(胶片)的访问实现，用于获知诸如输出的图片的分辨率
        等信息
    */
    PBRT_CPU_GPU inline Film GetFilm() const;

    /*
        模拟现实相机的快门效果，让胶片暴露在光中一小段时间。若此值不为0，会有动态模糊效果
        相对于相机有移动的物体会被模糊
        可以根据快门开关之间的时间内的光分布情况，利用蒙特卡洛积分和点采样方法，
        可以得到动态模糊的效果

        此接口用一个在[0,1)间随机均匀分布的样本u，对应到快门的开启时间点，一般来讲
        只是用来在快门开启和关闭时间里进行线性插值，使动态模糊更真实
    */
    PBRT_CPU_GPU inline Float SampleTime(Float u) const;

    /*
        允许相机设置ImageMetadata对象的参数，比如相机的转换矩阵等，若输出的图片的格式
        支持存储这些额外的信息，那么这些信息会被写入到最终图像里
    */
    void InitMetadata(ImageMetadata *metadata) const;

    /*
        Camera接口的实现类必须使CameraTransform类可以用于其他坐标空间
    */
    PBRT_CPU_GPU inline const CameraTransform &GetCameraTransform() const;

    PBRT_CPU_GPU
    void Approximate_dp_dxy(Point3f p, Normal3f n, Float time, int samplesPerPixel,
                            Vector3f *dpdx, Vector3f *dpdy) const;

    PBRT_CPU_GPU
    SampledSpectrum We(const Ray &ray, SampledWavelengths &lambda,
                       Point2f *pRasterOut = nullptr) const;

    PBRT_CPU_GPU
    void PDF_We(const Ray &ray, Float *pdfPos, Float *pdfDir) const;

    PBRT_CPU_GPU
    pstd::optional<CameraWiSample> SampleWi(const Interaction &ref, Point2f u,
                                            SampledWavelengths &lambda) const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_CAMERA_H
