// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_CAMERAS_H
#define PBRT_CAMERAS_H

#include <pbrt/pbrt.h>

#include <pbrt/base/camera.h>
#include <pbrt/base/film.h>
#include <pbrt/film.h>
#include <pbrt/interaction.h>
#include <pbrt/ray.h>
#include <pbrt/samplers.h>
#include <pbrt/util/image.h>
#include <pbrt/util/scattering.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace pbrt {

/*
    封装各个坐标空间之间的转换过程
    该类维护了两类转换：从相机空间到渲染空间，从渲染空间到世界空间
    Camera的实现类必须使此类支持其他系统的坐标空间

    这个类提供了各种重载函数，用于在各种坐标空间里变换。
*/
class CameraTransform {
  public:
    // CameraTransform Public Methods
    CameraTransform() = default;
    explicit CameraTransform(const AnimatedTransform &worldFromCamera);

    PBRT_CPU_GPU
    Point3f RenderFromCamera(Point3f p, Float time) const {
        return renderFromCamera(p, time);
    }
    PBRT_CPU_GPU
    Point3f CameraFromRender(Point3f p, Float time) const {
        return renderFromCamera.ApplyInverse(p, time);
    }
    PBRT_CPU_GPU
    Point3f RenderFromWorld(Point3f p) const { return worldFromRender.ApplyInverse(p); }

    PBRT_CPU_GPU
    Transform RenderFromWorld() const { return Inverse(worldFromRender); }
    PBRT_CPU_GPU
    Transform CameraFromRender(Float time) const {
        return Inverse(renderFromCamera.Interpolate(time));
    }
    PBRT_CPU_GPU
    Transform CameraFromWorld(Float time) const {
        return Inverse(worldFromRender * renderFromCamera.Interpolate(time));
    }

    PBRT_CPU_GPU
    bool CameraFromRenderHasScale() const { return renderFromCamera.HasScale(); }

    PBRT_CPU_GPU
    Vector3f RenderFromCamera(Vector3f v, Float time) const {
        return renderFromCamera(v, time);
    }

    PBRT_CPU_GPU
    Normal3f RenderFromCamera(Normal3f n, Float time) const {
        return renderFromCamera(n, time);
    }

    PBRT_CPU_GPU
    Ray RenderFromCamera(const Ray &r) const { return renderFromCamera(r); }

    PBRT_CPU_GPU
    RayDifferential RenderFromCamera(const RayDifferential &r) const {
        return renderFromCamera(r);
    }

    PBRT_CPU_GPU
    Vector3f CameraFromRender(Vector3f v, Float time) const {
        return renderFromCamera.ApplyInverse(v, time);
    }

    PBRT_CPU_GPU
    Normal3f CameraFromRender(Normal3f v, Float time) const {
        return renderFromCamera.ApplyInverse(v, time);
    }

    /*
        动画效果只有在相机空间中才能进行，在渲染空间进行有损性能
    */
    PBRT_CPU_GPU
    const AnimatedTransform &RenderFromCamera() const { return renderFromCamera; }

    /*
        不能动画化
    */
    PBRT_CPU_GPU
    const Transform &WorldFromRender() const { return worldFromRender; }

    std::string ToString() const;

  private:
    // CameraTransform Private Members
    AnimatedTransform renderFromCamera;
    Transform worldFromRender;
};

// CameraWiSample Definition
struct CameraWiSample {
    // CameraWiSample Public Methods
    CameraWiSample() = default;
    PBRT_CPU_GPU
    CameraWiSample(const SampledSpectrum &Wi, const Vector3f &wi, Float pdf,
                   Point2f pRaster, const Interaction &pRef, const Interaction &pLens)
        : Wi(Wi), wi(wi), pdf(pdf), pRaster(pRaster), pRef(pRef), pLens(pLens) {}

    SampledSpectrum Wi;
    Vector3f wi;
    Float pdf;
    Point2f pRaster;
    Interaction pRef, pLens;
};

/*
    包含了光线和光谱权重，简单的相机模型中，光谱权重默认为1，但是对于复杂的相机模型，
    比如RealisticCamera，为了模拟辐射度量学下的图像生成过程，权重就不是1
*/
struct CameraRay {
    Ray ray;
    SampledSpectrum weight = SampledSpectrum(1);
};

/*
    与CameraRay基本等同，只是用于存储带有微分量的光线
*/ 
struct CameraRayDifferential {
    RayDifferential ray;
    SampledSpectrum weight = SampledSpectrum(1);
};

/*
    CameraTransForm: 最重要的类，把相机坐标变换成场景所用的坐标
    
    shutterOpen, shutterClose: 快门的开关时间

    Film: 存储最终图像，且模拟了胶片的传感器

    Medium: 相机要考虑的介质
*/
struct CameraBaseParameters {
    CameraTransform cameraTransform;
    Float shutterOpen = 0, shutterClose = 1;
    Film film;
    Medium medium;
    CameraBaseParameters() = default;
    CameraBaseParameters(const CameraTransform &cameraTransform, Film film, Medium medium,
                         const ParameterDictionary &parameters, const FileLoc *loc);
};

/*
    camera接口的通用功能实现在此，所有camera实现须继承此类
*/
class CameraBase {
  public:
    // CameraBase Public Methods
    PBRT_CPU_GPU
    Film GetFilm() const { return film; }
    PBRT_CPU_GPU
    const CameraTransform &GetCameraTransform() const { return cameraTransform; }

    PBRT_CPU_GPU
    Float SampleTime(Float u) const { return Lerp(u, shutterOpen, shutterClose); }

    void InitMetadata(ImageMetadata *metadata) const;
    std::string ToString() const;

    PBRT_CPU_GPU
    void Approximate_dp_dxy(Point3f p, Normal3f n, Float time, int samplesPerPixel,
                            Vector3f *dpdx, Vector3f *dpdy) const {
        // Compute tangent plane equation for ray differential intersections
        Point3f pCamera = CameraFromRender(p, time);
        Transform DownZFromCamera =
            RotateFromTo(Normalize(Vector3f(pCamera)), Vector3f(0, 0, 1));
        Point3f pDownZ = DownZFromCamera(pCamera);
        Normal3f nDownZ = DownZFromCamera(CameraFromRender(n, time));
        Float d = nDownZ.z * pDownZ.z;

        // Find intersection points for approximated camera differential rays
        Ray xRay(Point3f(0, 0, 0) + minPosDifferentialX,
                 Vector3f(0, 0, 1) + minDirDifferentialX);
        Float tx = -(Dot(nDownZ, Vector3f(xRay.o)) - d) / Dot(nDownZ, xRay.d);
        Ray yRay(Point3f(0, 0, 0) + minPosDifferentialY,
                 Vector3f(0, 0, 1) + minDirDifferentialY);
        Float ty = -(Dot(nDownZ, Vector3f(yRay.o)) - d) / Dot(nDownZ, yRay.d);
        Point3f px = xRay(tx), py = yRay(ty);

        // Estimate $\dpdx$ and $\dpdy$ in tangent plane at intersection point
        Float sppScale =
            GetOptions().disablePixelJitter
                ? 1
                : std::max<Float>(.125, 1 / std::sqrt((Float)samplesPerPixel));
        *dpdx =
            sppScale * RenderFromCamera(DownZFromCamera.ApplyInverse(px - pDownZ), time);
        *dpdy =
            sppScale * RenderFromCamera(DownZFromCamera.ApplyInverse(py - pDownZ), time);
    }

  protected:
    // CameraBase Protected Members
    // 相机的位置和朝向
    CameraTransform cameraTransform;
    // 快门开关
    Float shutterOpen, shutterClose;
    // 胶片
    Film film;
    // 介质
    Medium medium;
    // 相机x,y位置的最小差分量
    Vector3f minPosDifferentialX, minPosDifferentialY;
    // 相机朝向的x,y最小差分量
    Vector3f minDirDifferentialX, minDirDifferentialY;

    // CameraBase Protected Methods
    CameraBase() = default;

    CameraBase(CameraBaseParameters p);

    /*
        通过多次调用camera的GenerateRay()函数来计算光线的微分量
        这个函数签名和Camera接口里的不同，此函数会在实现类里调用，这么做的原因是Camera不是用
        虚表实现的，所以需要自己写调用
    */
    PBRT_CPU_GPU
    static pstd::optional<CameraRayDifferential> GenerateRayDifferential(
        Camera camera, CameraSample sample, SampledWavelengths &lambda);

    PBRT_CPU_GPU
    Ray RenderFromCamera(const Ray &r) const {
        return cameraTransform.RenderFromCamera(r);
    }

    PBRT_CPU_GPU
    RayDifferential RenderFromCamera(const RayDifferential &r) const {
        return cameraTransform.RenderFromCamera(r);
    }

    PBRT_CPU_GPU
    Vector3f RenderFromCamera(Vector3f v, Float time) const {
        return cameraTransform.RenderFromCamera(v, time);
    }

    PBRT_CPU_GPU
    Normal3f RenderFromCamera(Normal3f v, Float time) const {
        return cameraTransform.RenderFromCamera(v, time);
    }

    PBRT_CPU_GPU
    Point3f RenderFromCamera(Point3f p, Float time) const {
        return cameraTransform.RenderFromCamera(p, time);
    }

    PBRT_CPU_GPU
    Vector3f CameraFromRender(Vector3f v, Float time) const {
        return cameraTransform.CameraFromRender(v, time);
    }

    PBRT_CPU_GPU
    Normal3f CameraFromRender(Normal3f v, Float time) const {
        return cameraTransform.CameraFromRender(v, time);
    }

    PBRT_CPU_GPU
    Point3f CameraFromRender(Point3f p, Float time) const {
        return cameraTransform.CameraFromRender(p, time);
    }

    void FindMinimumDifferentials(Camera camera);
};

// ProjectiveCamera Definition
/*
    投影相机： 子类包括正交相机和透视相机
*/
class ProjectiveCamera : public CameraBase {
  public:
    // ProjectiveCamera Public Methods
    ProjectiveCamera() = default;
    void InitMetadata(ImageMetadata *metadata) const;

    std::string BaseToString() const;

    /*
        baseParameters: 相机基本参数
        screenFromCamera: 从相机空间转到屏幕空间的变换矩阵
        screenWindow: 屏幕窗口的范围(从左下到右上，中央是原点)(屏幕空间)
        lensRadius: 镜片半径，0即为小孔相机(没有聚散焦效果)
        focalDistance: 镜头离近平面距离，模拟镜片效果时需要
    */
    ProjectiveCamera(CameraBaseParameters baseParameters,
                     const Transform &screenFromCamera, Bounds2f screenWindow,
                     Float lensRadius, Float focalDistance)
        : CameraBase(baseParameters),
          screenFromCamera(screenFromCamera),
          lensRadius(lensRadius),
          focalDistance(focalDistance) {
        // Compute projective camera transformations
        // Compute projective camera screen transformations
        // 屏幕空间原点在中心，要把坐标轴移动到左上角等效于原点往右下角移动
        // 然后再除以屏幕尺寸的宽高，把x,y限定在[0,1]
        Transform NDCFromScreen =
            Scale(1 / (screenWindow.pMax.x - screenWindow.pMin.x),
                  1 / (screenWindow.pMax.y - screenWindow.pMin.y), 1) *
            Translate(Vector3f(-screenWindow.pMin.x, -screenWindow.pMax.y, 0));
        // 光栅空间原点在左上角，这里y有个负号，因为之前缩放时没有把y倒过来
        Transform rasterFromNDC =
            Scale(film.FullResolution().x, -film.FullResolution().y, 1);
        rasterFromScreen = rasterFromNDC * NDCFromScreen;
        screenFromRaster = Inverse(rasterFromScreen);

        cameraFromRaster = Inverse(screenFromCamera) * screenFromRaster;
    }

  protected:
    // ProjectiveCamera Protected Members
    Transform screenFromCamera, cameraFromRaster;
    Transform rasterFromScreen, screenFromRaster;
    Float lensRadius, focalDistance;
};

// OrthographicCamera Definition
class OrthographicCamera : public ProjectiveCamera {
  public:
    // OrthographicCamera Public Methods
    OrthographicCamera(CameraBaseParameters baseParameters, Bounds2f screenWindow,
                       Float lensRadius, Float focalDist)
        : ProjectiveCamera(baseParameters, Orthographic(0, 1), screenWindow, lensRadius,
                           focalDist) {
        // Compute differential changes in origin for orthographic camera rays
        dxCamera = cameraFromRaster(Vector3f(1, 0, 0));
        dyCamera = cameraFromRaster(Vector3f(0, 1, 0));

        // Compute minimum differentials for orthographic camera
        minDirDifferentialX = minDirDifferentialY = Vector3f(0, 0, 0);
        minPosDifferentialX = dxCamera;
        minPosDifferentialY = dyCamera;
    }

    PBRT_CPU_GPU
    pstd::optional<CameraRay> GenerateRay(CameraSample sample,
                                          SampledWavelengths &lambda) const;

    PBRT_CPU_GPU
    pstd::optional<CameraRayDifferential> GenerateRayDifferential(
        CameraSample sample, SampledWavelengths &lambda) const;

    static OrthographicCamera *Create(const ParameterDictionary &parameters,
                                      const CameraTransform &cameraTransform, Film film,
                                      Medium medium, const FileLoc *loc,
                                      Allocator alloc = {});

    PBRT_CPU_GPU
    SampledSpectrum We(const Ray &ray, SampledWavelengths &lambda,
                       Point2f *pRaster2 = nullptr) const {
        LOG_FATAL("We() unimplemented for OrthographicCamera");
        return {};
    }

    PBRT_CPU_GPU
    void PDF_We(const Ray &ray, Float *pdfPos, Float *pdfDir) const {
        LOG_FATAL("PDF_We() unimplemented for OrthographicCamera");
    }

    PBRT_CPU_GPU
    pstd::optional<CameraWiSample> SampleWi(const Interaction &ref, Point2f u,
                                            SampledWavelengths &lambda) const {
        LOG_FATAL("SampleWi() unimplemented for OrthographicCamera");
        return {};
    }

    std::string ToString() const;

  private:
    // OrthographicCamera Private Members
    Vector3f dxCamera, dyCamera;
};

// PerspectiveCamera Definition
class PerspectiveCamera : public ProjectiveCamera {
  public:
    // PerspectiveCamera Public Methods
    PerspectiveCamera(CameraBaseParameters baseParameters, Float fov,
                      Bounds2f screenWindow, Float lensRadius, Float focalDist)
        : ProjectiveCamera(baseParameters, Perspective(fov, 1e-2f, 1000.f), screenWindow,
                           lensRadius, focalDist) {
        // Compute differential changes in origin for perspective camera rays
        dxCamera =
            cameraFromRaster(Point3f(1, 0, 0)) - cameraFromRaster(Point3f(0, 0, 0));
        dyCamera =
            cameraFromRaster(Point3f(0, 1, 0)) - cameraFromRaster(Point3f(0, 0, 0));

        // Compute _cosTotalWidth_ for perspective camera
        /*
            这个余弦值点乘观察向量，再与这个余弦值比较，
            可以用于快速剔除不在视锥体范围内的物体
        */ 
        Point2f radius = Point2f(film.GetFilter().Radius());
        Point3f pCorner(-radius.x, -radius.y, 0.f);
        Vector3f wCornerCamera = Normalize(Vector3f(cameraFromRaster(pCorner)));
        cosTotalWidth = wCornerCamera.z;
        DCHECK_LT(.9999 * cosTotalWidth, std::cos(Radians(fov / 2)));

        // Compute image plane area at $z=1$ for _PerspectiveCamera_
        Point2i res = film.FullResolution();
        Point3f pMin = cameraFromRaster(Point3f(0, 0, 0));
        Point3f pMax = cameraFromRaster(Point3f(res.x, res.y, 0));
        pMin /= pMin.z;
        pMax /= pMax.z;
        A = std::abs((pMax.x - pMin.x) * (pMax.y - pMin.y));

        // Compute minimum differentials for _PerspectiveCamera_
        FindMinimumDifferentials(this);
    }

    PerspectiveCamera() = default;

    static PerspectiveCamera *Create(const ParameterDictionary &parameters,
                                     const CameraTransform &cameraTransform, Film film,
                                     Medium medium, const FileLoc *loc,
                                     Allocator alloc = {});

    PBRT_CPU_GPU
    pstd::optional<CameraRay> GenerateRay(CameraSample sample,
                                          SampledWavelengths &lambda) const;

    PBRT_CPU_GPU
    pstd::optional<CameraRayDifferential> GenerateRayDifferential(
        CameraSample sample, SampledWavelengths &lambda) const;

    PBRT_CPU_GPU
    SampledSpectrum We(const Ray &ray, SampledWavelengths &lambda,
                       Point2f *pRaster2 = nullptr) const;
    PBRT_CPU_GPU
    void PDF_We(const Ray &ray, Float *pdfPos, Float *pdfDir) const;
    PBRT_CPU_GPU
    pstd::optional<CameraWiSample> SampleWi(const Interaction &ref, Point2f u,
                                            SampledWavelengths &lambda) const;

    std::string ToString() const;

  private:
    // PerspectiveCamera Private Members
    Vector3f dxCamera, dyCamera;
    Float cosTotalWidth;
    Float A;
};

// SphericalCamera Definition
class SphericalCamera : public CameraBase {
  public:
    // SphericalCamera::Mapping Definition
    enum Mapping { EquiRectangular, EqualArea };

    // SphericalCamera Public Methods
    SphericalCamera(CameraBaseParameters baseParameters, Mapping mapping)
        : CameraBase(baseParameters), mapping(mapping) {
        // Compute minimum differentials for _SphericalCamera_
        FindMinimumDifferentials(this);
    }

    static SphericalCamera *Create(const ParameterDictionary &parameters,
                                   const CameraTransform &cameraTransform, Film film,
                                   Medium medium, const FileLoc *loc,
                                   Allocator alloc = {});

    PBRT_CPU_GPU
    pstd::optional<CameraRay> GenerateRay(CameraSample sample,
                                          SampledWavelengths &lambda) const;

    PBRT_CPU_GPU
    pstd::optional<CameraRayDifferential> GenerateRayDifferential(
        CameraSample sample, SampledWavelengths &lambda) const {
        return CameraBase::GenerateRayDifferential(this, sample, lambda);
    }

    PBRT_CPU_GPU
    SampledSpectrum We(const Ray &ray, SampledWavelengths &lambda,
                       Point2f *pRaster2 = nullptr) const {
        LOG_FATAL("We() unimplemented for SphericalCamera");
        return {};
    }

    PBRT_CPU_GPU
    void PDF_We(const Ray &ray, Float *pdfPos, Float *pdfDir) const {
        LOG_FATAL("PDF_We() unimplemented for SphericalCamera");
    }

    PBRT_CPU_GPU
    pstd::optional<CameraWiSample> SampleWi(const Interaction &ref, Point2f u,
                                            SampledWavelengths &lambda) const {
        LOG_FATAL("SampleWi() unimplemented for SphericalCamera");
        return {};
    }

    std::string ToString() const;

  private:
    // SphericalCamera Private Members
    Mapping mapping;
};

// ExitPupilSample Definition
struct ExitPupilSample {
    Point3f pPupil;
    Float pdf;
};

// RealisticCamera Definition
class RealisticCamera : public CameraBase {
  public:
    // RealisticCamera Public Methods
    RealisticCamera(CameraBaseParameters baseParameters,
                    std::vector<Float> &lensParameters, Float focusDistance,
                    Float apertureDiameter, Image apertureImage, Allocator alloc);

    static RealisticCamera *Create(const ParameterDictionary &parameters,
                                   const CameraTransform &cameraTransform, Film film,
                                   Medium medium, const FileLoc *loc,
                                   Allocator alloc = {});

    PBRT_CPU_GPU
    pstd::optional<CameraRay> GenerateRay(CameraSample sample,
                                          SampledWavelengths &lambda) const;

    PBRT_CPU_GPU
    pstd::optional<CameraRayDifferential> GenerateRayDifferential(
        CameraSample sample, SampledWavelengths &lambda) const {
        return CameraBase::GenerateRayDifferential(this, sample, lambda);
    }

    PBRT_CPU_GPU
    SampledSpectrum We(const Ray &ray, SampledWavelengths &lambda,
                       Point2f *pRaster2 = nullptr) const {
        LOG_FATAL("We() unimplemented for RealisticCamera");
        return {};
    }

    PBRT_CPU_GPU
    void PDF_We(const Ray &ray, Float *pdfPos, Float *pdfDir) const {
        LOG_FATAL("PDF_We() unimplemented for RealisticCamera");
    }

    PBRT_CPU_GPU
    pstd::optional<CameraWiSample> SampleWi(const Interaction &ref, Point2f u,
                                            SampledWavelengths &lambda) const {
        LOG_FATAL("SampleWi() unimplemented for RealisticCamera");
        return {};
    }

    std::string ToString() const;

  private:
    // RealisticCamera Private Declarations
    struct LensElementInterface {
        Float curvatureRadius;
        Float thickness;
        Float eta;
        Float apertureRadius;
        std::string ToString() const;
    };

    // RealisticCamera Private Methods
    PBRT_CPU_GPU
    Float LensRearZ() const { return elementInterfaces.back().thickness; }

    PBRT_CPU_GPU
    Float LensFrontZ() const {
        Float zSum = 0;
        for (const LensElementInterface &element : elementInterfaces)
            zSum += element.thickness;
        return zSum;
    }

    PBRT_CPU_GPU
    Float RearElementRadius() const { return elementInterfaces.back().apertureRadius; }

    PBRT_CPU_GPU
    Float TraceLensesFromFilm(const Ray &rCamera, Ray *rOut) const;

    PBRT_CPU_GPU
    static bool IntersectSphericalElement(Float radius, Float zCenter, const Ray &ray,
                                          Float *t, Normal3f *n) {
        // Compute _t0_ and _t1_ for ray--element intersection
        Point3f o = ray.o - Vector3f(0, 0, zCenter);
        Float A = ray.d.x * ray.d.x + ray.d.y * ray.d.y + ray.d.z * ray.d.z;
        Float B = 2 * (ray.d.x * o.x + ray.d.y * o.y + ray.d.z * o.z);
        Float C = o.x * o.x + o.y * o.y + o.z * o.z - radius * radius;
        Float t0, t1;
        if (!Quadratic(A, B, C, &t0, &t1))
            return false;

        // Select intersection $t$ based on ray direction and element curvature
        bool useCloserT = (ray.d.z > 0) ^ (radius < 0);
        *t = useCloserT ? std::min(t0, t1) : std::max(t0, t1);
        if (*t < 0)
            return false;

        // Compute surface normal of element at ray intersection point
        *n = Normal3f(Vector3f(o + *t * ray.d));
        *n = FaceForward(Normalize(*n), -ray.d);

        return true;
    }

    PBRT_CPU_GPU
    Float TraceLensesFromScene(const Ray &rCamera, Ray *rOut) const;

    void DrawLensSystem() const;
    void DrawRayPathFromFilm(const Ray &r, bool arrow, bool toOpticalIntercept) const;
    void DrawRayPathFromScene(const Ray &r, bool arrow, bool toOpticalIntercept) const;

    static void ComputeCardinalPoints(Ray rIn, Ray rOut, Float *p, Float *f);
    void ComputeThickLensApproximation(Float pz[2], Float f[2]) const;
    Float FocusThickLens(Float focusDistance);
    Bounds2f BoundExitPupil(Float filmX0, Float filmX1) const;
    void RenderExitPupil(Float sx, Float sy, const char *filename) const;

    PBRT_CPU_GPU
    pstd::optional<ExitPupilSample> SampleExitPupil(Point2f pFilm, Point2f uLens) const;

    void TestExitPupilBounds() const;

    // RealisticCamera Private Members
    Bounds2f physicalExtent;
    pstd::vector<LensElementInterface> elementInterfaces;
    Image apertureImage;
    pstd::vector<Bounds2f> exitPupilBounds;
};

PBRT_CPU_GPU inline pstd::optional<CameraRay> Camera::GenerateRay(CameraSample sample,
                                                     SampledWavelengths &lambda) const {
    auto generate = [&](auto ptr) { return ptr->GenerateRay(sample, lambda); };
    return Dispatch(generate);
}

PBRT_CPU_GPU inline Film Camera::GetFilm() const {
    auto getfilm = [&](auto ptr) { return ptr->GetFilm(); };
    return Dispatch(getfilm);
}

PBRT_CPU_GPU inline Float Camera::SampleTime(Float u) const {
    auto sample = [&](auto ptr) { return ptr->SampleTime(u); };
    return Dispatch(sample);
}

PBRT_CPU_GPU inline const CameraTransform &Camera::GetCameraTransform() const {
    auto gtc = [&](auto ptr) -> const CameraTransform & {
        return ptr->GetCameraTransform();
    };
    return Dispatch(gtc);
}

PBRT_CPU_GPU inline void Camera::Approximate_dp_dxy(Point3f p, Normal3f n, Float time,
                                       int samplesPerPixel, Vector3f *dpdx,
                                       Vector3f *dpdy) const {
    if constexpr (AllInheritFrom<CameraBase>(Camera::Types())) {
        return ((const CameraBase *)ptr())
            ->Approximate_dp_dxy(p, n, time, samplesPerPixel, dpdx, dpdy);
    } else {
        auto approx = [&](auto ptr) {
            return ptr->Approximate_dp_dxy(p, n, time, samplesPerPixel, dpdx, dpdy);
        };
        return Dispatch(approx);
    }
}

}  // namespace pbrt

#endif  // PBRT_CAMERAS_H
