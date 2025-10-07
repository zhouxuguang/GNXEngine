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
    //static bool PointInSphere(const Vector3f& point, const Sphere& sphere);

    //计算球离点最近的点
    //static Vector3f ClosestPoint(const Sphere& sphere, const Vector3f& point);

    //判断点是否在AABB中
    static bool PointInAABB(const Vector3f& point, const AABB& aabb);

    //计算点离AABB最近的点
    static Vector3f ClosestPoint(const AABB& aabb, const Vector3f& point);

    //判断点是否在OBB中
    static bool PointInOBB(const Vector3f& point, const OBB& obb);

	//计算点离OBB最近的点
	static Vector3f ClosestPoint(const OBB& obb, const Vector3f& point);
};


NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_POINTTEST_INCLUDE_MNFDBHFGHF */
