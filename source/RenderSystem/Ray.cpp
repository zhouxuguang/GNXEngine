//
//  Ray.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#include "Ray.h"

NS_RENDERSYSTEM_BEGIN

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
	//    matrix.transformPoint(&_origin);
//    matrix.transformVector(&_direction);
//    _direction.normalize();
}

template class Ray<float>;
template class Ray<double>;

NS_RENDERSYSTEM_END
