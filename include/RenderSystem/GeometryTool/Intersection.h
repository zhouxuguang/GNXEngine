//
//  Intersection.hpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#ifndef GNX_ENGINE_INTERSECTION_INCLUDE
#define GNX_ENGINE_INTERSECTION_INCLUDE

#include "RSDefine.h"
#include "MathUtil/Vector3.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

class Ray;
class Sphere;
class AABB;

// Intersects a Ray with a triangle.
bool IntersectRayTriangle (const Ray& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c);
// t is to be non-Null and returns the first intersection point of the ray (ray.o + t * ray.dir)
bool IntersectRayTriangle (const Ray& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c, float* t);

// Intersects a ray with a volume.
// Returns true if the ray stats inside the volume or in front of the volume
bool IntersectRaySphere (const Ray& ray, const Sphere& inSphere);
bool IntersectRayAABB (const Ray& ray, const AABB& inAABB);

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_INTERSECTION_INCLUDE */
