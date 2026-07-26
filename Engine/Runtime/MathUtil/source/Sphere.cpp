//
//  Sphere.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#include "Sphere.h"

NS_MATHUTIL_BEGIN

template <typename T>
bool Sphere<T>::IsInside(const Sphere<T>& inSphere) const
{
    T sqrDist = (GetCenter() - inSphere.GetCenter()).Length();
    if (GetRadius() * GetRadius() > sqrDist + inSphere.GetRadius() * inSphere.GetRadius())
    {
        return true;
    }

    return false;
}

template <typename T>
Sphere<T> Sphere<T>::Merge(const Sphere<T>& a, const Sphere<T>& b)
{
    Vector3<T> dir = b.mCenter - a.mCenter;
    T d = dir.Length();

    // A 包含 B
    if (a.mRadius >= d + b.mRadius)
        return a;

    // B 包含 A
    if (b.mRadius >= d + a.mRadius)
        return b;

    // 球心重合（d == 0）
    if (d < T(1e-8))
        return (a.mRadius >= b.mRadius) ? a : b;

    // 精确最小合并
    T rM = (a.mRadius + d + b.mRadius) * T(0.5);
    T t  = (rM - a.mRadius) / d;
    return Sphere<T>(a.mCenter + dir * t, rM);
}

template class Sphere<float>;
template class Sphere<double>;

NS_MATHUTIL_END
