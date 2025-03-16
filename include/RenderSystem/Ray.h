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
class Ray
{
public:
    /**
     * 构造一个新的射线
     *
     * @param origin 起点
     * @param direction The ray's direction.
     */
    Ray(const Vector3f& origin, const Vector3f& direction);

    ~Ray();
    
    Vector3f GetOrigin() const;
    
    Vector3f GetDirection() const;

    /**
     * 通过给定的参数计算射线上的点
     */
    Vector3f PointFromDistance(double distance) const;

    /**
     * 通过变换矩阵变换射线
     *
     * @param matrix The transformation matrix to transform by.
     */
    void Transform(const Matrix4x4f& matrix);

    Vector3f mOrigin;        // 射线起点
    Vector3f mDirection;     // 射线方向
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_RAY_INCLUDE_MDEEVGFDGV */
