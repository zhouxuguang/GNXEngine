//
//  Ray.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#ifndef GNXENGINE_RENDERSYSTEM_RAY_INCLUDE_MDEEVGFDGV
#define GNXENGINE_RENDERSYSTEM_RAY_INCLUDE_MDEEVGFDGV

#include "RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector3.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

/**
 * 射线类
 **/
template <typename T>
class Ray
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

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_RAY_INCLUDE_MDEEVGFDGV */
