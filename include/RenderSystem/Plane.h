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
class Plane
{
public:
    /**
    * 从三个点创建一个平面
    */
    Plane(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3);

    /**
    * 从法向量和距离创建平面
    */
    Plane(const Vector3f& normal, float dist);

    /**
    * create plane from normal and a point on plane.
    */
    Plane(const Vector3f& normal, const Vector3f& point);
    
    /**
     * create a default plan whose normal is (0, 0, 1), and _dist is 0, xoy plan in fact.
     */
    Plane();
    
    /**
    * init plane from tree point.
    */
    void initPlane(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3);

    /**
    * init plane from normal and dist.
    */
    void initPlane(const Vector3f& normal, float dist);

    /**
    * init plane from normal and a point on plane.
    */
    void initPlane(const Vector3f& normal, const Vector3f& point);

    /**
    * dist to plane, > 0 normal direction
    */
    float dist2Plane(const Vector3f& p) const;

    /**
    * Gets the plane's normal.
    */
    const Vector3f& getNormal() const { return mNormal; }

    /**
    * Gets the plane's distance to the origin along its normal.
    */
    float getDist() const  { return mDist; }

    /**
    * Return the side where the point is.
    */
    PointSide getSide(const Vector3f& point) const;

protected:
    Vector3f mNormal;         // the normal line of the plane
    float mDist;             // original displacement of the normal
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_PLANE_INCLUDE_MFFDF */
