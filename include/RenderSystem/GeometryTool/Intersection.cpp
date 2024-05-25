//
//  Intersection.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#include "Intersection.h"
#include "Ray.h"
#include "Sphere.h"

NS_RENDERSYSTEM_BEGIN

bool IntersectRayTriangle (const Ray& ray, const Vector3f& a, const Vector3f& b, const Vector3f& c)
{
    float t;
    return IntersectRayTriangle (ray, a, b, c, &t);
}

bool IntersectRayTriangle (const Ray& ray, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, float* outT)
{

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

bool IntersectRaySphere (const Ray& ray, const Sphere& inSphere)
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

NS_RENDERSYSTEM_END
