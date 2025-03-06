//
//  Plane.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#ifndef GNXENGINE_RENDERSYSTEM_PLANE_INCLUDE_MFFDF
#define GNXENGINE_RENDERSYSTEM_PLANE_INCLUDE_MFFDF

#include "RSDefine.h"
#include "MathUtil/Vector3.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

enum PointSide
{
    IN_PLANE,
    FRONT_PLANE,
    BEHIND_PLANE,
};

/**
    平面定义
 **/
template<typename T>
class Plane
{
public:
    /**
    * 从三个点创建一个平面
    */
    Plane(const Vector3<T>& p1, const Vector3<T>& p2, const Vector3<T>& p3);

    /**
    * 从法向量和距离创建平面
    */
    Plane(const Vector3<T>& normal, T dist);

    /**
    * 根据平面上的一个点和发线创建平面
    */
    Plane(const Vector3<T>& normal, const Vector3<T>& point);
    
    /**
     * 创建默认的平面
     */
    Plane();

    /**
    * dist to plane, > 0 normal direction
    */
    T dist2Plane(const Vector3<T>& p) const;

    /**
    * Gets the plane's normal.
    */
    const Vector3<T>& getNormal() const { return mNormal; }

    /**
    * Gets the plane's distance to the origin along its normal.
    */
    T getDist() const  { return mDist; }

    /**
    * Return the side where the point is.
    */
    PointSide getSide(const Vector3<T>& point) const;

protected:
    Vector3<T> mNormal;         // the normal line of the plane
    T mDist;             // original displacement of the normal
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_PLANE_INCLUDE_MFFDF */
