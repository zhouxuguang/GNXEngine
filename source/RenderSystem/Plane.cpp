//
//  Plane.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#include "Plane.h"

NS_RENDERSYSTEM_BEGIN

template<typename T>
Plane<T>::Plane() : mNormal(0, 0, 1), mDist(0)
{
}

template<typename T>
Plane<T>::Plane(const Vector3<T>& p1, const Vector3<T>& p2, const Vector3<T>& p3)
{
	Vector3f p21 = p2 - p1;
	Vector3f p32 = p3 - p2;
	mNormal = Vector3f::CrossProduct(p21, p32);
	mNormal.Normalize();
	mDist = mNormal.DotProduct(p1);
}

template<typename T>
Plane<T>::Plane(const Vector3<T>& normal, T dist)
{
	T oneOverLength = 1 / normal.Length();
	mNormal = normal * oneOverLength;
	mDist = dist * oneOverLength;
}

template<typename T>
Plane<T>::Plane(const Vector3<T>& normal, const Vector3<T>& point)
{
	mNormal = normal;
	mNormal.Normalize();
	mDist = mNormal.DotProduct(point);
}

template<typename T>
T Plane<T>::dist2Plane(const Vector3<T>& p) const
{
    return mNormal.DotProduct(p) - mDist;
}

template<typename T>
PointSide Plane<T>::getSide(const Vector3<T>& point) const
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
