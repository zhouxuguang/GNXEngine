//
//  Intersection.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#include "Intersection.h"
#include "Ray.h"
#include "Sphere.h"
#include "PointTest.h"

NS_RENDERSYSTEM_BEGIN

bool IntersectRayTriangle (const Ray& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c)
{
    float t;
    return IntersectRayTriangle(ray, a, b, c, &t);
}

bool IntersectRayTriangle(const Ray& ray, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, float* outT)
{
    // 算法参考论文 https://www.tandfonline.com/doi/abs/10.1080/10867651.1997.10487468
    Vector3f e1 = v1 - v0;
    Vector3f e2 = v2 - v0;
    Vector3f s = ray.getOrigin() - v0;
    Vector3f s1 = Vector3f::CrossProduct(ray.getDirection(), e2);
    Vector3f s2 = Vector3f::CrossProduct(s, e1);
    
    float det = s1.DotProduct(e1);
    
    Vector3f result = Vector3f(s2.DotProduct(e2), s1.DotProduct(s), s2.DotProduct(ray.getDirection()));
    result = result / det;
    
    if (result.x <= 0)
    {
        return false;
    }
    
    if (result.y < 0 || result.y > 1 || result.z < 0 || result.z > 1)
    {
        return false;
    }
    
    *outT = result.x;
    
    return true;
}

bool IntersectRaySphere(const Ray& ray, const Sphere& inSphere)
{
    Vector3f dif = inSphere.GetCenter () - ray.getOrigin ();
    float d = dif.DotProduct(ray.getDirection ());
    float lSqr = dif.DotProduct(dif);
    float rSqr = inSphere.GetRadius() * inSphere.GetRadius();

    if (d < 0.0F && lSqr > rSqr)
        return false;

    float mSqr = lSqr - d * d;

    if (mSqr > rSqr)
        return false;
    else
        return true;
}

bool IntersectRayAABB(const Ray& ray, const AABB& inAABB)
{
    return false;
}

bool IntersectSphereSphere(const Sphere& s1, const Sphere& s2)
{
	float radiiSum = s1.mRadius + s2.mRadius;
	float sqDistance = (s1.mCenter - s2.mCenter).LengthSq();
	return sqDistance < radiiSum * radiiSum;
}

bool IntersectSphereAABB(const Sphere& sphere, const AABB& aabb)
{
	Vector3f closestPoint = PointTest::ClosestPoint(aabb, sphere.mCenter);
	float distSq = (sphere.mCenter - closestPoint).LengthSq();
	float radiusSq = sphere.mRadius * sphere.mRadius;
	return distSq < radiusSq;
}

bool IntersectSphereOBB(const Sphere& sphere, const OBB& obb)
{
	Vector3f closestPoint = PointTest::ClosestPoint(obb, sphere.mCenter);
	float distSq = (sphere.mCenter - closestPoint).LengthSq();
	float radiusSq = sphere.mRadius * sphere.mRadius;
	return distSq < radiusSq;
}

bool IntersectAABBAABB(const AABB& aabb1, const AABB& aabb2)
{
	Vector3f aMin = aabb1.mMin;
    Vector3f aMax = aabb1.mMax;
    Vector3f bMin = aabb2.mMin;
    Vector3f bMax = aabb2.mMax;

	return	(aMin.x <= bMax.x && aMax.x >= bMin.x) &&
		(aMin.y <= bMax.y && aMax.y >= bMin.y) &&
		(aMin.z <= bMax.z && aMax.z >= bMin.z);
}

NS_RENDERSYSTEM_END
