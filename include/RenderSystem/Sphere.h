//
//  Sphere.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/11/6.
//

#ifndef GNX_ENGINE_SPHERE_INCLUDE
#define GNX_ENGINE_SPHERE_INCLUDE

#include "RSDefine.h"
#include "MathUtil/Vector3.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

class Sphere
{
public:
    Vector3f    mCenter;
    float       mRadius;
    
public:
    
    Sphere () {}
    Sphere (const Vector3f& p0, float r)                {Set (p0, r);}
    
    void Set (const Vector3f& p0)                        {mCenter = p0;    mRadius = 0;}
    void Set (const Vector3f& p0, float r)                {mCenter = p0;    mRadius = r;}
    
    Vector3f& GetCenter () {return mCenter;}
    const Vector3f& GetCenter () const  {return mCenter;}
    
    float& GetRadius () {return mRadius;}
    const float& GetRadius ()const {return mRadius;}
    
    bool IsInside(const Sphere& inSphere)const;
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SPHERE_INCLUDE */
