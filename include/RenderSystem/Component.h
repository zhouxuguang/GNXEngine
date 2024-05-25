//
//  Component.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef GNXENGINE_COMPONENT_INCLUDE_DSDG
#define GNXENGINE_COMPONENT_INCLUDE_DSDG

#include "Camera.h"
#include "Light.h"
#include "MathUtil/Vector3.h"
#include "MathUtil/Quaternion.h"
#include "Transform.h"

NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

class SceneNode;

class Component
{
public:
    Component();
    
    virtual ~Component();
    
    virtual void Update(float deltaTime) {}
    
    void SetSceneNode(SceneNode* sceneNode)
    {
        mSceneNode = sceneNode;
    }
    
protected:
    SceneNode* mSceneNode = nullptr;
};

//变换组件
class TransformComponent : public Component
{
public:
    TransformComponent(const Transform& transform)
    {
        this->transform = transform;
    }
    
    ~TransformComponent()
    {
    }
    
public:
    Transform transform;
};


//相机组件
class CameraComponent : public Component
{
public:
    CameraComponent();
    
    ~CameraComponent();
    
    Camera& GetCamera()
    {
        return mCamera;
    }
    
private:
    Camera mCamera;
};

class LightComponent : public Component
{
public:
    LightComponent();
    
    ~LightComponent();
    
    Light* GetLightPtr()
    {
        return mLight;
    }
    
    void SetLightPtr(Light* light)
    {
        mLight = light;
    }
    
private:
    Light *mLight = nullptr;
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_COMPONENT_INCLUDE_DSDG */
