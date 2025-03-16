//
//  Ray.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#include "Ray.h"

NS_RENDERSYSTEM_BEGIN

Ray::Ray(const Vector3f& origin, const Vector3f& direction)
{
    mOrigin = origin;
    mDirection = direction;
}

Ray::~Ray()
{
}

Vector3f Ray::GetOrigin() const
{
    return mOrigin;
}

Vector3f Ray::GetDirection() const
{
    return mDirection;
}

Vector3f Ray::PointFromDistance(double distance) const
{
    return this->mOrigin + this->mDirection * distance;
}

void Ray::Transform(const Matrix4x4f& matrix)
{
//    matrix.transformPoint(&_origin);
//    matrix.transformVector(&_direction);
//    _direction.normalize();
}

NS_RENDERSYSTEM_END
