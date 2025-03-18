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

bool IntersectRayTriangle (const Rayf& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c)
{
    float t;
    return IntersectRayTriangle(ray, a, b, c, &t);
}

bool IntersectRayTriangle(const Rayf& ray, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, float* outT)
{
    // 算法参考论文 https://www.tandfonline.com/doi/abs/10.1080/10867651.1997.10487468
    Vector3f e1 = v1 - v0;
    Vector3f e2 = v2 - v0;
    Vector3f s = ray.GetOrigin() - v0;
    Vector3f s1 = Vector3f::CrossProduct(ray.GetDirection(), e2);
    Vector3f s2 = Vector3f::CrossProduct(s, e1);
    
    float det = s1.DotProduct(e1);
    
    Vector3f result = Vector3f(s2.DotProduct(e2), s1.DotProduct(s), s2.DotProduct(ray.GetDirection()));
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

bool IntersectRaySphere(const Rayf& ray, const Sphere& inSphere)
{
    Vector3f dif = inSphere.GetCenter () - ray.GetOrigin ();
    float d = dif.DotProduct(ray.GetDirection ());
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

bool IntersectRayAABB(const Rayf& ray, const AABB& inAABB)
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

#if 0


int TestOBBOBB(OBB& a, OBB& b)
{
    float ra, rb;
    Matrix33 R, AbsR;

    // Compute rotation matrix expressing b in a's coordinate frame
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R[i][j] = Dot(a.u[i], b.u[j]);

    // Compute translation vector t
    Vector t = b.c - a.c;
    // Bring translation into a's coordinate frame
    t = Vector(Dot(t, a.u[0]), Dot(t, a.u[1]), Dot(t, a.u[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null (see text for details)
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            AbsR[i][j] = Abs(R[i][j]) + EPSILON;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a.e[i];
        rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
        if (Abs(t[i]) > ra + rb) return 0;
    }

    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
        rb = b.e[i];
        if (Abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return 0;
    }

    // Test axis L = A0 x B0
    ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
    rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
    if (Abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return 0;

    // Test axis L = A0 x B1
    ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
    rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
    if (Abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return 0;

    // Test axis L = A0 x B2
    ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
    rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
    if (Abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return 0;

    // Test axis L = A1 x B0
    ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
    rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
    if (Abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return 0;

    // Test axis L = A1 x B1
    ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
    rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
    if (Abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return 0;

    // Test axis L = A1 x B2
    ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
    rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
    if (Abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return 0;

    // Test axis L = A2 x B0
    ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
    rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
    if (Abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return 0;

    // Test axis L = A2 x B1
    ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
    rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
    if (Abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return 0;

    // Test axis L = A2 x B2
    ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
    rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
    if (Abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return 0;

    // Since no separating axis found, the OBBs must be intersecting
    return 1;
}

#endif

NS_RENDERSYSTEM_END
