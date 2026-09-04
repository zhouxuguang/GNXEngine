//
//  Plane.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#include "Plane.h"

NS_MATHUTIL_BEGIN

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
    // 与 GetPointDistance 保持一致（带符号距离 = n·P + mDist）
    // BUG 修复：原来是 n·P - mDist，与 GetPointDistance 相差 2*mDist，
    // 导致平面内部自相矛盾（用哪个构造函数创建平面，结果都对不上）。
    return mNormal.DotProduct(p) + mDist;
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
	// 平面方程 n·P + mDist = 0，mDist = -n·p1
	// BUG 修复：原来写成 +n·p1，导致 GetPointDistance/ProjectPointOntoPlane 结果错误。
	mDist = -mNormal.DotProduct(p1);
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
	T oneOverLength = T(1.0 / mNormal.Length());
	mNormal = mNormal * oneOverLength;
	// BUG 修复：原来只归一化法线却不缩放 w。
	// 平面方程 n·P + d = 0 两侧同除 |n|，d 也必须按相同比例缩放，
	// 否则用未归一化 Vector4 构造的平面距离/投影全是错的。
	mDist = coff.w * oneOverLength;
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

NS_MATHUTIL_END
