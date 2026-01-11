//
//  Ray.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#ifndef GNXENGINE_MATHUTIL_RAY_INCLUDE_MDEEVGFDGV
#define GNXENGINE_MATHUTIL_RAY_INCLUDE_MDEEVGFDGV

#include "Math3DCommon.h"
#include "Matrix4x4.h"
#include "Vector3.h"

NS_MATHUTIL_BEGIN

/**
 * 射线类
 **/
template <typename T>
class MATH3D_API Ray
{
public:
    /**
     * 构造一个新的射线
     *
     * @param origin 起点
     * @param direction The ray's direction.
     */
    Ray(const Vector3<T>& origin, const Vector3<T>& direction);

    ~Ray();

    Vector3<T> GetOrigin() const;

    Vector3<T> GetDirection() const;

    /**
     * 通过给定的参数计算射线上的点
     */
    Vector3<T> PointFromDistance(T distance) const;

    /**
     * 通过变换矩阵变换射线
     *
     * @param matrix The transformation matrix to transform by.
     */
    void Transform(const Matrix4x4<T>& matrix);

    Ray<T> operator-() const;

    Vector3<T> mOrigin;        // 射线起点
    Vector3<T> mDirection;     // 射线方向
};

typedef Ray<float> Rayf;
typedef Ray<double> Rayd;

NS_MATHUTIL_END

#endif /* GNXENGINE_MATHUTIL_RAY_INCLUDE_MDEEVGFDGV */
