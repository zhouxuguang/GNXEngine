//
//  Ray.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#include "Ray.h"

NS_RENDERSYSTEM_BEGIN

Ray::Ray() : mDirection(0, 0, 1)
{
}

Ray::Ray(const Ray& ray)
{
    set(ray.mOrigin, ray.mDirection);
}

Ray::Ray(const Vector3f& origin, const Vector3f& direction)
{
    set(origin, direction);
}

Ray::~Ray()
{
}

Vector3f Ray::getOrigin() const
{
    return mOrigin;
}

Vector3f Ray::getDirection() const
{
    return mDirection;
}

bool Ray::intersects(const AABB& box, float* distance) const
{
#if 0
    float lowt = 0.0f;
    float t;
    bool hit = false;
    Vector3f hitpoint;
    const Vector3f& min = box.mMin;
    const Vector3f& max = box.mMax;
    const Vector3f& rayorig = mOrigin;
    const Vector3f& raydir = mDirection;
    
    // Check origin inside first
    if (rayorig > min && rayorig < max)
        return true;
    
    // Check each face in turn, only check closest 3
    // Min x
    if (rayorig.x <= min.x && raydir.x > 0)
    {
        t = (min.x - rayorig.x) / raydir.x;
        if (t >= 0)
        {
            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if (hitpoint.y >= min.y && hitpoint.y <= max.y &&
                hitpoint.z >= min.z && hitpoint.z <= max.z &&
                (!hit || t < lowt))
            {
                hit = true;
                lowt = t;
            }
        }
    }
    // Max x
    if (rayorig.x >= max.x && raydir.x < 0)
    {
        t = (max.x - rayorig.x) / raydir.x;
        if (t >= 0)
        {
            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if (hitpoint.y >= min.y && hitpoint.y <= max.y &&
                hitpoint.z >= min.z && hitpoint.z <= max.z &&
                (!hit || t < lowt))
            {
                hit = true;
                lowt = t;
            }
        }
    }
    // Min y
    if (rayorig.y <= min.y && raydir.y > 0)
    {
        t = (min.y - rayorig.y) / raydir.y;
        if (t >= 0)
        {
            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                hitpoint.z >= min.z && hitpoint.z <= max.z &&
                (!hit || t < lowt))
            {
                hit = true;
                lowt = t;
            }
        }
    }
    // Max y
    if (rayorig.y >= max.y && raydir.y < 0)
    {
        t = (max.y - rayorig.y) / raydir.y;
        if
            
            
            (t >= 0)
        {
            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                hitpoint.z >= min.z && hitpoint.z <= max.z &&
                (!hit || t < lowt))
            {
                hit = true;
                lowt = t;
            }
        }
    }
    // Min z
    if (rayorig.z <= min.z && raydir.z > 0)
    {
        t = (min.z - rayorig.z) / raydir.z;
        if (t >= 0)
        {
            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                hitpoint.y >= min.y && hitpoint.y <= max.y &&
                (!hit || t < lowt))
            {
                hit = true;
                lowt = t;
            }
        }
    }
    // Max z
    if (rayorig.z >= max.z && raydir.z < 0)
    {
        t = (max.z - rayorig.z) / raydir.z;
        if (t >= 0)
        {
            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if (hitpoint.x >= min.x && hitpoint.x <= max.x &&
                hitpoint.y >= min.y && hitpoint.y <= max.y &&
                (!hit || t < lowt))
            {
                hit = true;
                lowt = t;
            }
        }
    }
    
    if (distance)
        *distance = lowt;
    
    return hit;
    
#endif
    
    return false;
}

bool Ray::intersects(const OBB& obb, float* distance) const
{
#if 0
    AABB aabb;
    aabb._min = - obb._extents;
    aabb._max = obb._extents;
    
    Ray ray;
    ray._direction = _direction;
    ray._origin = _origin;
    
    Mat4 mat = Mat4::IDENTITY;
    mat.m[0] = obb._xAxis.x;
    mat.m[1] = obb._xAxis.y;
    mat.m[2] = obb._xAxis.z;
    
    mat.m[4] = obb._yAxis.x;
    mat.m[5] = obb._yAxis.y;
    mat.m[6] = obb._yAxis.z;
    
    mat.m[8] = obb._zAxis.x;
    mat.m[9] = obb._zAxis.y;
    mat.m[10] = obb._zAxis.z;
    
    mat.m[12] = obb._center.x;
    mat.m[13] = obb._center.y;
    mat.m[14] = obb._center.z;
    
    mat = mat.getInversed();
    
    ray.transform(mat);
    
    return ray.intersects(aabb, distance);
#endif
    return false;
    
}

float Ray::dist(const Plane& plane) const
{
    float ndd = plane.getNormal().DotProduct(mDirection);
    if(ndd == 0)
        return 0.0f;
    float ndo = plane.getNormal().DotProduct(mOrigin);
    return (plane.getDist() - ndo) / ndd;
}

Vector3f Ray::intersects(const Plane& plane) const
{
    float dis = this->dist(plane);
    return mOrigin + dis * mDirection;
}

void Ray::set(const Vector3f& origin, const Vector3f& direction)
{
    mOrigin = origin;
    mDirection = direction;
    mDirection.Normalize();
}

void Ray::transform(const Matrix4x4f& matrix)
{
//    matrix.transformPoint(&_origin);
//    matrix.transformVector(&_direction);
//    _direction.normalize();
}

NS_RENDERSYSTEM_END
