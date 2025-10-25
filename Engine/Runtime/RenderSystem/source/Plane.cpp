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
    InitPlane(p1, p2, p3);
}

template<typename T>
Plane<T>::Plane(const Vector3<T>& normal, T dist)
{
    InitPlane(normal, dist);
}

template<typename T>
Plane<T>::Plane(const Vector3<T>& normal, const Vector3<T>& point)
{
    InitPlane(normal, point);
}

template<typename T>
Plane<T>::Plane(const Vector4<T>& coff)
{
    InitPlane(coff);
}

template<typename T>
T Plane<T>::Dist2Plane(const Vector3<T>& p) const
{
    return mNormal.DotProduct(p) - mDist;
}

template<typename T>
PointSide Plane<T>::GetSide(const Vector3<T>& point) const
{
    T dist = Dist2Plane(point);
    if (dist > 0)
        return PointSide::FRONT_PLANE;
    else if (dist < 0)
        return PointSide::BEHIND_PLANE;
    else
        return PointSide::IN_PLANE;
}

template<typename T>
void Plane<T>::InitPlane(const Vector3<T>& p1, const Vector3<T>& p2, const Vector3<T>& p3)
{
	Vector3<T> p21 = p2 - p1;
	Vector3<T> p32 = p3 - p2;
	mNormal = Vector3<T>::CrossProduct(p21, p32);
	mNormal.Normalize();
	mDist = mNormal.DotProduct(p1);
}

template<typename T>
void Plane<T>::InitPlane(const Vector3<T>& normal, T dist)
{
	T oneOverLength = T(1.0 / normal.Length());
	mNormal = normal * oneOverLength;
	mDist = dist * oneOverLength;
}

template<typename T>
void Plane<T>::InitPlane(const Vector3<T>& normal, const Vector3<T>& point)
{
	//-glm::dot(normal, point)  这里容易出错, 参考cesium native的平面类
    InitPlane(normal, -normal.DotProduct(point));
}

template<typename T>
void Plane<T>::InitPlane(const Vector4<T>& coff)
{
	mNormal = Vector3<T>(coff.x, coff.y, coff.z);
	mNormal.Normalize();
	mDist = coff.w;
}

template<typename T>
T Plane<T>::GetPointDistance(const Vector3<T>& point) const
{
	return this->mNormal.DotProduct(point) + this->mDist;
}

template<typename T>
Vector3<T> Plane<T>::ProjectPointOntoPlane(const Vector3<T>& point) const
{
    // projectedPoint = point - (normal.point + scale) * normal
    const T pointDistance = this->GetPointDistance(point);
    const Vector3<T> scaledNormal = this->mNormal * pointDistance;
    return point - scaledNormal;
}

template class Plane<float>;
template class Plane<double>;

NS_RENDERSYSTEM_END
