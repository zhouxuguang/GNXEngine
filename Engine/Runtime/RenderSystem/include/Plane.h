//
//  Plane.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/1.
//

#ifndef GNXENGINE_RENDERSYSTEM_PLANE_INCLUDE_MFFDF
#define GNXENGINE_RENDERSYSTEM_PLANE_INCLUDE_MFFDF

#include "RSDefine.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Vector4.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

enum PointSide
{
    IN_PLANE,        //在平面上
    FRONT_PLANE,     //在法向量的正向一侧
    BEHIND_PLANE,    //在法向量的反向一侧
};

/**
    平面定义
 **/
template<typename T>
class RENDERSYSTEM_API Plane
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
	* 根据平面上的四个系数创建平面方程
	*/
	Plane(const Vector4<T>& coff);
    
    /**
     * 创建默认的平面
     */
    Plane();

    /**
    * dist to plane, > 0 normal direction
    */
    T Dist2Plane(const Vector3<T>& p) const;

    /**
    * Gets the plane's normal.
    */
    const Vector3<T>& GetNormal() const { return mNormal; }

    /**
    * Gets the plane's distance to the origin along its normal.
    */
    T GetDist() const { return mDist; }

    /**
    * Return the side where the point is.
    */
    PointSide GetSide(const Vector3<T>& point) const;

    /**
    * init plane from tree point.
    */
    void InitPlane(const Vector3<T>& p1, const Vector3<T>& p2, const Vector3<T>& p3);

    /**
    * init plane from normal and dist.
    */
    void InitPlane(const Vector3<T>& normal, T dist);

    /**
    * init plane from normal and a point on plane.
    */
    void InitPlane(const Vector3<T>& normal, const Vector3<T>& point);

	/**
	* 根据平面上的四个系数创建平面方程
	*/
    void InitPlane(const Vector4<T>& coff);

    /**
    * 获得点到平面的最近的带符号距离
    *
    * @param point The point.
    * @returns 点到平面的最近的带符号距离
    */
    T GetPointDistance(const Vector3<T>& point) const;

    /**
     * @brief 将三维点投影到该平面上
     * 
     * @param point 被投影的点
     * @returns 投影后的点
     */
    Vector3<T> ProjectPointOntoPlane(const Vector3<T>& point) const;

private:
    Vector3<T> mNormal;         // 平面法线
    T mDist;             // original displacement of the normal
};

typedef Plane<float> Planef;
typedef Plane<double> Planed;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_RENDERSYSTEM_PLANE_INCLUDE_MFFDF */
