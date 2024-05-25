//
//  Light.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/29.
//

#include "Light.h"

NS_RENDERSYSTEM_BEGIN

Light::Light()
{
}

Light::Light(const std::string& name)
{
    mName = name;
}

Light::~Light()
{
    //
}

const Vector3f & Light::getPosition(void) const
{
    return mPosition;
}

void Light::setPosition(const Vector3f & position)
{
    mPosition = position;
}

const Vector3f & Light::getColor() const
{
    return mColor;
}

void Light::setColor(const Vector3f & color)
{
    mColor = color;
}

const Vector3f & Light::getStrength() const
{
    return mStrength;
}

void Light::setStrength(const Vector3f& strength)
{
    mStrength = strength;
}

float Light::getFalloffStart() const
{
    return mFalloffStart;
}

void Light::setFalloffStart(float falloffStart)
{
    mFalloffStart = falloffStart;
}

float Light::getFalloffEnd() const
{
    return mFalloffEnd;
}

void Light::setFalloffEnd(float falloffEnd)
{
    mFalloffEnd = falloffEnd;
}

const std::string& Light::getName() const
{
    return mName;
}

///

PointLight::PointLight()
{
}

PointLight::PointLight(const std::string &name) : Light(name)
{
}

PointLight::~PointLight()
{
}

Light::LightType PointLight::getLightType() const
{
    return LightType::PointLight;
}

///

SpotLight::SpotLight()
{
    //
}

SpotLight::SpotLight(const std::string &name) : Light(name)
{
    //
}

SpotLight::~SpotLight()
{
}

float SpotLight::getSpotPower() const
{
    return mSpotPower;
}

void SpotLight::setSpotPower(float spotPower)
{
    mSpotPower = spotPower;
}

Light::LightType SpotLight::getLightType() const
{
    return LightType::SpotLight;
}

///

DirectionLight::DirectionLight()
{
}

DirectionLight::DirectionLight(const std::string &name) : Light(name)
{
    //
}

DirectionLight::~DirectionLight()
{
}

const Vector3f& DirectionLight::getDirection() const
{
    return mDirection;
}

void DirectionLight::setDirection(const Vector3f& direction)
{
    mDirection = direction;
}

Light::LightType DirectionLight::getLightType() const
{
    return LightType::DirectionLight;
}

NS_RENDERSYSTEM_END
