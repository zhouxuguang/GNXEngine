//
//  Ray.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#include "Ray.h"
#include <cmath>

NS_MATHUTIL_BEGIN

template<typename T>
Ray<T>::Ray(const Vector3<T>& origin, const Vector3<T>& direction)
{
    mOrigin = origin;
    mDirection = direction;
}

template<typename T>
Ray<T>::~Ray()
{
}

template<typename T>
Vector3<T> Ray<T>::GetOrigin() const
{
    return mOrigin;
}

template<typename T>
Vector3<T> Ray<T>::GetDirection() const
{
    return mDirection;
}

template<typename T>
Vector3<T> Ray<T>::PointFromDistance(T distance) const
{
    return this->mOrigin + this->mDirection * distance;
}

template<typename T>
void Ray<T>::Transform(const Matrix4x4<T>& matrix)
{
    // BUG 修复：原实现整个函数体被注释掉，调用后射线不发生任何变化。
    // 原点按点(w=1)变换，方向按向量(w=0)变换（不受平移影响）。
    const Vector4<T> transformedOrigin = matrix * Vector4<T>(mOrigin.x, mOrigin.y, mOrigin.z, T(1));
    const Vector4<T> transformedDir    = matrix * Vector4<T>(mDirection.x, mDirection.y, mDirection.z, T(0));

    // 透视矩阵下需要做透视除法；若 w 为 0（点被投影到无穷远）则保持原值避免除零
    if (std::abs(transformedOrigin.w) > T(1e-12))
    {
        mOrigin.x = transformedOrigin.x / transformedOrigin.w;
        mOrigin.y = transformedOrigin.y / transformedOrigin.w;
        mOrigin.z = transformedOrigin.z / transformedOrigin.w;
    }

    mDirection.x = transformedDir.x;
    mDirection.y = transformedDir.y;
    mDirection.z = transformedDir.z;
    mDirection.Normalize();
}

template<typename T>
Ray<T> Ray<T>::operator-() const
{
    return Ray(this->mOrigin, -mDirection);
}

template class Ray<float>;
template class Ray<double>;

NS_MATHUTIL_END
