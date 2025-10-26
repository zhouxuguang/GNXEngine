//
//  Sphere.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#ifndef GNX_ENGINE_SPHERE_INCLUDE
#define GNX_ENGINE_SPHERE_INCLUDE

#include "RSDefine.h"
#include "Runtime/MathUtil/include/Vector3.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

template <typename T>
class Sphere
{
public:
    Vector3<T>    mCenter;
    T       mRadius;
    
public:
    
    Sphere() {}
    Sphere(const Vector3<T>& p0, T r)
    {
        Set(p0, r);
    }
    
    void Set(const Vector3<T>& p0)
    {
        mCenter = p0;
        mRadius = 0;
    }
    
    void Set(const Vector3<T>& p0, T r)
    {
        mCenter = p0;
        mRadius = r;
    }
    
    Vector3<T>& GetCenter() { return mCenter; }
    const Vector3<T>& GetCenter() const { return mCenter; }
    
    T& GetRadius() { return mRadius; }
    const T& GetRadius() const { return mRadius; }
    
    bool IsInside(const Sphere& inSphere) const;
};

typedef Sphere<float> Spheref;
typedef Sphere<double> Sphered;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SPHERE_INCLUDE */
