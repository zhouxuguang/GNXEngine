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
	initPlane(p1, p2, p3);
}

template<typename T>
Plane<T>::Plane(const Vector3<T>& normal, T dist)
{
	initPlane(normal, dist);
}

template<typename T>
Plane<T>::Plane(const Vector3<T>& normal, const Vector3<T>& point)
{
	initPlane(normal, point);
}

template<typename T>
Plane<T>::Plane(const Vector4<T>& coff)
{
	initPlane(coff);
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

template<typename T>
void Plane<T>::initPlane(const Vector3<T>& p1, const Vector3<T>& p2, const Vector3<T>& p3)
{
	Vector3<T> p21 = p2 - p1;
	Vector3<T> p32 = p3 - p2;
	mNormal = Vector3<T>::CrossProduct(p21, p32);
	mNormal.Normalize();
	mDist = mNormal.DotProduct(p1);
}

template<typename T>
void Plane<T>::initPlane(const Vector3<T>& normal, T dist)
{
	T oneOverLength = 1.0 / normal.Length();
	mNormal = normal * oneOverLength;
	mDist = dist * oneOverLength;
}

template<typename T>
void Plane<T>::initPlane(const Vector3<T>& normal, const Vector3<T>& point)
{
	//-glm::dot(normal, point)  这里容易出错, 参考cesium native的平面类
	initPlane(normal, -normal.DotProduct(point));
}

template<typename T>
void Plane<T>::initPlane(const Vector4<T>& coff)
{
	mNormal = Vector3<T>(coff.x, coff.y, coff.z);
	mNormal.Normalize();
	mDist = coff.w;
}

template<typename T>
T Plane<T>::getPointDistance(const Vector3<T>& point) const
{
	return this->mNormal.DotProduct(point) + this->mDist;
}

template class Plane<float>;
template class Plane<double>;

NS_RENDERSYSTEM_END
