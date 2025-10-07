//
//  PointTest.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/30.
//

#ifndef GNXENGINE_RENDERSYSTEM_POINTTEST_INCLUDE_MNFDBHFGHF
#define GNXENGINE_RENDERSYSTEM_POINTTEST_INCLUDE_MNFDBHFGHF

#include "RSDefine.h"
#include "MathUtil/Matrix4x4.h"
#include "MathUtil/Vector3.h"
#include "RenderSystem/Sphere.h"
#include "RenderSystem/AABB.h"
#include "RenderSystem/OBB.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

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
        // Find a normalized vector from the center of the sphere to the test point
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
        result.y = (result.y < min.x) ? min.y : result.y;
        result.z = (result.z < min.x) ? min.z : result.z;

        result.x = (result.x > max.x) ? max.x : result.x;
        result.y = (result.y > max.x) ? max.y : result.y;
        result.z = (result.z > max.x) ? max.z : result.z;

        return result;
    }

    //判断点是否在OBB中
    template<typename T>
    static bool PointInOBB(const Vector3<T>& point, const OrientedBoundingBox<T>& obb)
    {
//        Vector3f dir = point - obb.mCenter;
//
//        for (int i = 0; i < 3; ++i)
//        {
//            const float* orientation = &obb.orientation.asArray[i * 3];
//            vec3 axis(orientation[0], orientation[1], orientation[2]);
//
//            float distance = Dot(dir, axis);
//
//            if (distance > obb.size.asArray[i])
//            {
//                return false;
//            }
//            if (distance < -obb.size.asArray[i])
//            {
//                return false;
//            }
//        }

        return true;
    }

	//计算点离OBB最近的点
    template<typename T>
	static Vector3<T> ClosestPoint(const OrientedBoundingBox<T>& obb, const Vector3<T>& point)
    {
        return Vector3<T>();
    }
};


NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_POINTTEST_INCLUDE_MNFDBHFGHF */
