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
    Vector3f    m_Center;
    float        m_Radius;
    
public:
    
    Sphere () {}
    Sphere (const Vector3f& p0, float r)                {Set (p0, r);}
    
    void Set (const Vector3f& p0)                        {m_Center = p0;    m_Radius = 0;}
    void Set (const Vector3f& p0, float r)                {m_Center = p0;    m_Radius = r;}
    
    Vector3f& GetCenter () {return m_Center;}
    const Vector3f& GetCenter () const  {return m_Center;}
    
    float& GetRadius () {return m_Radius;}
    const float& GetRadius ()const {return m_Radius;}
    
    bool IsInside(const Sphere& inSphere)const;
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SPHERE_INCLUDE */
