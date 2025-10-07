//
//  Intersection.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#ifndef GNX_ENGINE_INTERSECTION_INCLUDE
#define GNX_ENGINE_INTERSECTION_INCLUDE

#include "RSDefine.h"
#include "MathUtil/Vector3.h"
#include "Sphere.h"
#include "Ray.h"
#include "AABB.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

// Intersects a Ray with a triangle.
bool IntersectRayTriangle(const Rayf& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c);
// t is to be non-Null and returns the first intersection point of the ray (ray.o + t * ray.dir)
bool IntersectRayTriangle(const Rayf& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c, float* t);

// 判断射线和球是否相交
template<typename T>
bool IntersectRaySphere(const Ray<T>& ray, const Sphere<T>& inSphere);

// 判断射线和AABB是否相交
template<typename T>
bool IntersectRayAABB(const Ray<T>& ray, const AxisAlignedBox<T>& inAABB);

// 判断两个球体是否相交
template<typename T>
bool IntersectSphereSphere(const Sphere<T>& s1, const Sphere<T>& s2);

//判断球和AABB是否相交
//bool IntersectSphereAABB(const Sphere& sphere, const AABB& aabb);

//判断球和OBB是否相交
//bool IntersectSphereOBB(const Sphere& sphere, const OBB& obb);

//判断AABB之间是否相交
//bool IntersectAABBAABB(const AABB& aabb1, const AABB& aabb2);

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_INTERSECTION_INCLUDE */
