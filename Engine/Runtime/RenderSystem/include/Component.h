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
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Quaternion.h"
#include "Transform.h"

NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

class SceneNode;

// 组件类型枚举（手动分配，ID 稳定、可序列化）
enum class ComponentType : uint32_t
{
    Transform       = 0,
    Camera          = 1,
    Light           = 2,
    MeshRenderer    = 3,
    MeshFilter      = 4,
    SkinnedRenderer = 5,
    SkeletonAnim    = 6,
    Terrain         = 7,
    Count           = 8
};

/// 类型 → ComponentType 编译期映射（在各自头文件中特化）
template<typename T> struct ComponentTypeOf
{
    static constexpr ComponentType Value = ComponentType::Count;  // 默认
};

/// 运行时获取组件类型 ID（基于编译期映射）
template<typename T>
inline ComponentType GetComponentTypeID()
{
    return ComponentTypeOf<T>::Value;
}

class RENDERSYSTEM_API Component
{
public:
    Component();
    
    virtual ~Component();
    
    virtual void Update(float deltaTime) {}
    
    void SetSceneNode(SceneNode* sceneNode)
    {
        mSceneNode = sceneNode;
    }

    SceneNode* GetSceneNode() const
    {
        return mSceneNode;
    }
    
    /// 动态获取组件类型（虚函数，运行时安全）
    virtual ComponentType GetComponentType() const = 0;
    
protected:
    SceneNode* mSceneNode = nullptr;
};

class TransformComponent : public Component
{
public:
    TransformComponent(const Transform& transform)
    {
        this->transform = transform;
    }
    
    ~TransformComponent() {}
    
    ComponentType GetComponentType() const override { return ComponentType::Transform; }
    
public:
    Transform transform;
};

//变换组件
template<> struct ComponentTypeOf<TransformComponent>
{
    static constexpr ComponentType Value = ComponentType::Transform;
};

class CameraComponent : public Component
{
public:
    CameraComponent();
    
    ~CameraComponent();
    
    ComponentType GetComponentType() const override { return ComponentType::Camera; }
    
    Camera& GetCamera()
    {
        return mCamera;
    }
    
private:
    Camera mCamera;
};

//相机组件
template<> struct ComponentTypeOf<CameraComponent>
{
    static constexpr ComponentType Value = ComponentType::Camera;
};

class LightComponent : public Component
{
public:
    LightComponent();
    
    ~LightComponent();
    
    ComponentType GetComponentType() const override { return ComponentType::Light; }
    
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

template<> struct ComponentTypeOf<LightComponent>
{
    static constexpr ComponentType Value = ComponentType::Light;
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_COMPONENT_INCLUDE_DSDG */
