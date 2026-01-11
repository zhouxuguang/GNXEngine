//
//  Intersection.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#ifndef GNX_ENGINE_MATH_UTLS_INTERSECTION_INCLUDE
#define GNX_ENGINE_MATH_UTLS_INTERSECTION_INCLUDE

#include "Math3DCommon.h"
#include "Vector3.h"
#include "Sphere.h"
#include "Ray.h"
#include "AABB.h"
#include "OBB.h"

USING_NS_MATHUTIL

NS_MATHUTIL_BEGIN

// Intersects a Ray with a triangle.
bool IntersectRayTriangle(const Rayf& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c);
// t is to be non-Null and returns the first intersection point of the ray (ray.o + t * ray.dir)
bool IntersectRayTriangle(const Rayf& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c, float* t);

// 判断射线和球是否相交
template <typename T>
bool IntersectRaySphere(const Ray<T>& ray, const Sphere<T>& inSphere);

// 判断射线和AABB是否相交
template <typename T>
bool IntersectRayAABB(const Ray<T>& ray, const AxisAlignedBox<T>& inAABB);

// 判断两个球体是否相交
template <typename T>
bool IntersectSphereSphere(const Sphere<T>& s1, const Sphere<T>& s2);

//判断球和AABB是否相交
template <typename T>
bool IntersectSphereAABB(const Sphere<T>& sphere, const AxisAlignedBox<T>& aabb);

//判断球和OBB是否相交
template <typename T>
bool IntersectSphereOBB(const Sphere<T>& sphere, const OrientedBoundingBox<T>& obb);

//判断AABB之间是否相交
template <typename T>
bool IntersectAABBAABB(const AxisAlignedBox<T>& aabb1, const AxisAlignedBox<T>& aabb2);

NS_MATHUTIL_END

#endif /* GNX_ENGINE_MATH_UTLS_INTERSECTION_INCLUDE */
