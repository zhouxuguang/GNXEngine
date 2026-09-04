//
//  PointTest.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#ifndef GNXENGINE_MATHUTIL_POINTTEST_INCLUDE_MNFDBHFGHF
#define GNXENGINE_MATHUTIL_POINTTEST_INCLUDE_MNFDBHFGHF

#include "Math3DCommon.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Sphere.h"
#include "AABB.h"
#include "OBB.h"

NS_MATHUTIL_BEGIN

class PointTest
{
public:
	//判断点是否在球中
    template<typename T>
    static bool PointInSphere(const Vector3<T>& point, const Sphere<T>& sphere)
    {
        return (point - sphere.mCenter).LengthSq() < sphere.mRadius * sphere.mRadius;
    }

    //计算球离点最近的点
    template<typename T>
    static Vector3<T> ClosestPoint(const Sphere<T>& sphere, const Vector3<T>& point)
    {
        // Find a normalized vector from center of sphere to test point
        Vector3<T> sphereToPoint = point - sphere.mCenter;
        sphereToPoint.Normalize();

        // 使用半径缩放向量
        sphereToPoint = sphereToPoint * sphere.mRadius;

        //加上球心偏移
        return sphereToPoint + sphere.mCenter;
    }

    //判断点是否在AABB中
    template<typename T>
    static bool PointInAABB(const Vector3<T>& point, const AxisAlignedBox<T>& aabb)
    {
        const Vector3<T>& min = aabb.minimum;
        const Vector3<T>& max = aabb.maximum;

        if (point.x < min.x || point.y < min.y || point.z < min.z)
        {
            return false;
        }
        if (point.x > max.x || point.y > max.y || point.z > max.z)
        {
            return false;
        }

        return true;
    }

    //计算点离AABB最近的点
    template<typename T>
    static Vector3<T> ClosestPoint(const AxisAlignedBox<T>& aabb, const Vector3<T>& point)
    {
        Vector3<T> result = point;
        const Vector3<T>& min = aabb.minimum;
        const Vector3<T>& max = aabb.maximum;

        result.x = (result.x < min.x) ? min.x : result.x;
        result.y = (result.y < min.y) ? min.y : result.y;
        result.z = (result.z < min.z) ? min.z : result.z;

        result.x = (result.x > max.x) ? max.x : result.x;
        result.y = (result.y > max.y) ? max.y : result.y;
        result.z = (result.z > max.z) ? max.z : result.z;

        return result;
    }

    //判断点是否在OBB中
    template<typename T>
    static bool PointInOBB(const Vector3<T>& point, const OrientedBoundingBox<T>& obb)
    {
        // BUG 修复：原实现为被注释掉的代码并直接 return true（永远命中）。
        // 将点变换到 OBB 局部空间（单位盒 [-1,1]^3），再判断每个轴向上的分量是否越界。
        const Vector3<T> dir = point - obb.mCenter;
        const Vector3<T> local = obb.mInverseHalfAxes * dir;

        return (local.x >= -T(1)) && (local.x <= T(1)) &&
               (local.y >= -T(1)) && (local.y <= T(1)) &&
               (local.z >= -T(1)) && (local.z <= T(1));
    }

	//计算点离OBB最近的点
    template<typename T>
	static Vector3<T> ClosestPoint(const OrientedBoundingBox<T>& obb, const Vector3<T>& point)
    {
        // BUG 修复：原实现直接返回原点 (0,0,0)，导致所有基于它的测试（如
        // IntersectSphereOBB）把球心与整个世界原点比较。
        // 正确做法：将点到中心的向量变换到 OBB 局部空间，clamp 到 [-1,1]^3，
        // 再变换回世界空间并加上中心。
        const Vector3<T> dir = point - obb.mCenter;
        Vector3<T> local = obb.mInverseHalfAxes * dir;

        local.x = (local.x < -T(1)) ? -T(1) : (local.x > T(1) ? T(1) : local.x);
        local.y = (local.y < -T(1)) ? -T(1) : (local.y > T(1) ? T(1) : local.y);
        local.z = (local.z < -T(1)) ? -T(1) : (local.z > T(1) ? T(1) : local.z);

        return obb.mCenter + obb.mHalfAxes * local;
    }
};


NS_MATHUTIL_END

#endif /* GNXENGINE_MATHUTIL_POINTTEST_INCLUDE_MNFDBHFGHF */
