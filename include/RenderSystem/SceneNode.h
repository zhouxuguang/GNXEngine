//
//  SceneNode.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/29.
//

#ifndef GNX_ENGINE_SCENE_NODE_INCLUDE_H
#define GNX_ENGINE_SCENE_NODE_INCLUDE_H

#include "RSDefine.h"
//#include "RenderCore/VertexBuffer.h"
#include "RenderCore/RenderDevice.h"
#include "MathUtil/Vector3.h"
#include "MathUtil/Quaternion.h"
#include "SceneObject.h"
#include "Component.h"
#include <type_traits>

NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

//场景节点定义
class SceneNode
{
public:
    SceneNode();
    
    virtual ~SceneNode();
    
    virtual SceneNode * createChildSceneNode(const std::string &name,
                                             const Vector3f &translate = Vector3f::ZERO,
                                             const Quaternionf &rotate = Quaternionf::IDENTITY);
    
    virtual SceneNode * createRendererNode(const std::string &name,
                                           const std::string& filePath,
                                    const Vector3f &translate = Vector3f::ZERO,
                                    const Quaternionf &rotate = Quaternionf::IDENTITY,
                                           const Vector3f &scale = Vector3f::UNIT_SCALE);
    
    virtual void attachObject(SceneObject *obj);
    
    virtual void detachAllObjects(void);
    
    virtual void detachObject(SceneObject *obj);
    
    virtual SceneObject *detachObject(uint32_t index);
    
    const std::vector<SceneObject*>& getAllAttachedObjects() const;
    
    int GetComponentCount() const  { return (int)mComponents.size(); }
    Component* GetComponentPtrAtIndex(int i) const;
    
    // 根据组件的类型进行查询组件
    template<class T>
    T* QueryComponentT() const
    {
        for (int i = 0; i < mComponents.size(); i ++)
        {
            Component* p1 = mComponents[i];
//            typedef decltype(p1) CompPtrType;
//            if (std::is_same<CompPtrType, T*>::value)
//            {
//                return (T*)mComponents[i];
//            }
            
            if (dynamic_cast<T*>(p1))
            {
                return (T*)mComponents[i];
            }
        }
        
        return nullptr;
    }
    
    // 增加组件
    void AddComponent(Component* component)
    {
        component->SetSceneNode(this);
        mComponents.push_back(component);
    }
    
    void Update(float deltaTime);
    
    // 获得所有的子节点列表
    const std::vector<SceneNode*>& GetAllNodes() const;
    
private:
    std::vector<SceneNode*> mChildNodes;    //孩子节点
    SceneNode* mParentNode = nullptr;       //父亲节点
    std::vector<SceneObject*> mAttachedObjects;   //该节点关联的物体
    std::vector<Component*> mComponents;        //组件列表
};


NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SCENE_NODE_INCLUDE_H */
