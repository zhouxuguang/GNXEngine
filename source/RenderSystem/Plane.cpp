//
//  Plane.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#include "Plane.h"

NS_RENDERSYSTEM_BEGIN

Plane::Plane() : mNormal(0.f, 0.f, 1.f), mDist(0.f)
{
}

// create plane from tree point
Plane::Plane(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3)
{
    initPlane(p1, p2, p3);
}

// create plane from normal and dist
Plane::Plane(const Vector3f& normal, float dist)
{
    initPlane(normal, dist);
}

// create plane from normal and a point on plane
Plane::Plane(const Vector3f& normal, const Vector3f& point)
{
    initPlane(normal, point);
}

void Plane::initPlane(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3)
{
    Vector3f p21 = p2 - p1;
    Vector3f p32 = p3 - p2;
    mNormal = Vector3f::CrossProduct(p21, p32);
    mNormal.Normalize();
    mDist = mNormal.DotProduct(p1);
}

void Plane::initPlane(const Vector3f& normal, float dist)
{
    float oneOverLength = 1 / normal.Length();
    mNormal = normal * oneOverLength;
    mDist = dist * oneOverLength;
}

void Plane::initPlane(const Vector3f& normal, const Vector3f& point)
{
    mNormal = normal;
    mNormal.Normalize();
    mDist = mNormal.DotProduct(point);
}

float Plane::dist2Plane(const Vector3f& p) const
{
    return mNormal.DotProduct(p) - mDist;
}


PointSide Plane::getSide(const Vector3f& point) const
{
    float dist = dist2Plane(point);
    if (dist > 0)
        return PointSide::FRONT_PLANE;
    else if (dist < 0)
        return PointSide::BEHIND_PLANE;
    else
        return PointSide::IN_PLANE;
}

NS_RENDERSYSTEM_END
