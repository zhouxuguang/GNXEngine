//
//  Sphere.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#include "Sphere.h"
#include <cmath>
#include <cassert>
#include <cstddef>
#include <limits>
#include <vector>

NS_MATHUTIL_BEGIN

// Jack Ritter. "An Efficient Bounding Sphere." 1990
template <typename T>
static void ComputeBoundingSphere(T result[4], const T points[][3], size_t count)
{
    assert(count > 0);

    size_t pmin[3] = {};
    size_t pmax[3] = {};
    T minVal[3] = { std::numeric_limits<T>::max(), std::numeric_limits<T>::max(), std::numeric_limits<T>::max() };
    T maxVal[3] = { std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest() };

    for (size_t i = 0; i < count; ++i)
    {
        const T* p = points[i];
        for (int axis = 0; axis < 3; ++axis)
        {
            if (p[axis] < minVal[axis]) { minVal[axis] = p[axis]; pmin[axis] = i; }
            if (p[axis] > maxVal[axis]) { maxVal[axis] = p[axis]; pmax[axis] = i; }
        }
    }

    T paxisd2 = T(0);
    int paxis = 0;
    for (int axis = 0; axis < 3; ++axis)
    {
        const T* p1 = points[pmin[axis]];
        const T* p2 = points[pmax[axis]];
        T d2 = (p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]) + (p2[2] - p1[2]) * (p2[2] - p1[2]);
        if (d2 > paxisd2) { paxisd2 = d2; paxis = axis; }
    }

    const T* p1 = points[pmin[paxis]];
    const T* p2 = points[pmax[paxis]];
    T center[3] = { (p1[0] + p2[0]) / T(2), (p1[1] + p2[1]) / T(2), (p1[2] + p2[2]) / T(2) };
    T radius = std::sqrt(paxisd2) / T(2);

    for (size_t i = 0; i < count; ++i)
    {
        const T* p = points[i];
        T dx = p[0] - center[0], dy = p[1] - center[1], dz = p[2] - center[2];
        T d2 = dx * dx + dy * dy + dz * dz;

        if (d2 > radius * radius)
        {
            T d = std::sqrt(d2);
            assert(d > T(0));
            T newRadius = (d + radius) / T(2);
            T k = (newRadius - radius) / d;               // fraction to move center
            center[0] += dx * k;
            center[1] += dy * k;
            center[2] += dz * k;
            radius = newRadius;
        }
    }

    result[0] = center[0];
    result[1] = center[1];
    result[2] = center[2];
    result[3] = radius;
}

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
Sphere<T> Sphere<T>::FromPositions(const std::vector<Vector3<T>>& positions)
{
    if (positions.empty())
	{
		return Sphere<T>(Vector3<T>(T(0), T(0), T(0)), T(0));
	}

    T result[4];
    ComputeBoundingSphere<T>(result, reinterpret_cast<const T(*)[3]>(positions.data()), positions.size());
    return Sphere<T>(Vector3<T>(result[0], result[1], result[2]), result[3]);
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
