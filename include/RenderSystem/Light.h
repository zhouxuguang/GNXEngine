//
//  Light.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/29.
//

#ifndef GNX_ENGINE_LIGHT_INCLUDE_HPP_FFGVFDGVKF
#define GNX_ENGINE_LIGHT_INCLUDE_HPP_FFGVFDGVKF

#include "RSDefine.h"
#include "MathUtil/Vector3.h"
#include "SceneObject.h"

NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

//光源基类
class Light : public SceneObject
{
public:
    //光源类型
    enum LightType
    {
        PointLight = 0,
        SpotLight = 1,
        DirectionLight = 2
    };
    
    Light();
    
    explicit Light(const std::string &name);
     
    virtual ~Light();
    
    const Vector3f &getPosition(void) const;
    
    void setPosition(const Vector3f & position);
    
    const Vector3f & getColor() const;
    
    void setColor(const Vector3f & color);
    
    const Vector3f & getStrength() const;
    
    void setStrength(const Vector3f& strength);
    
    float getFalloffStart() const;
    
    void setFalloffStart(float falloffStart);
    
    float getFalloffEnd() const;
    
    void setFalloffEnd(float falloffEnd);
    
    const std::string& getName() const;
    
    virtual LightType getLightType() const = 0;
    
private:
    Vector3f mPosition;
    Vector3f mColor;
    Vector3f mStrength;
    
    float mFalloffStart;   //线性衰减的起点
    float mFalloffEnd;    //线性衰减的终点
    
    std::string mName;
};

//点光源
class PointLight : public Light
{
public:
    PointLight();
    
    explicit PointLight(const std::string &name);
    
    ~PointLight();
    
    LightType getLightType() const;
    
private:
};

//聚光灯
class SpotLight : public Light
{
public:
    SpotLight();
    
    explicit SpotLight(const std::string &name);
    
    ~SpotLight();
    
    float getSpotPower() const;
    
    void setSpotPower(float spotPower);
    
    LightType getLightType() const;
    
private:
    float mSpotPower;
};

//平行光
class DirectionLight : public Light
{
public:
    DirectionLight();
    
    explicit DirectionLight(const std::string &name);
    
    ~DirectionLight();
    
    const Vector3f& getDirection() const;
    
    void setDirection(const Vector3f& direction);
    
    LightType getLightType() const;
    
private:
    Vector3f mDirection;
};


NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_LIGHT_INCLUDE_HPP_FFGVFDGVKF */
