//
//  SceneNode.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/29.
//

#ifndef GNX_ENGINE_SCENE_NODE_INCLUDE_H
#define GNX_ENGINE_SCENE_NODE_INCLUDE_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Quaternion.h"
#include "SceneObject.h"
#include "Component.h"
#include <type_traits>
#include <algorithm>
#include <string>

NS_RENDERSYSTEM_BEGIN

using namespace mathutil;

//场景节点定义
class RENDERSYSTEM_API SceneNode
{
public:
    SceneNode();

    virtual ~SceneNode();

    // 获取节点名称
    const std::string& GetName() const { return mName; }

    // 设置节点名称
    void SetName(const std::string& name) { mName = name; }

    // 获取父节点
    SceneNode* GetParent() const { return mParentNode; }

    // 查找子节点（仅直接子节点）
    SceneNode* FindChild(const std::string& name) const;

    // 递归查找所有子孙节点（包括子节点的子节点）
    SceneNode* FindChildRecursive(const std::string& name) const;

    // 获取所有子孙节点（递归）
    std::vector<SceneNode*> GetAllDescendants() const;

    // 移除子节点（不会删除节点，只是断开父子关系）
    void RemoveChild(SceneNode* child);

    // 销毁子节点（从场景树中删除并释放）
    void DestroyChild(SceneNode* child);

    virtual SceneNode *CreateChildSceneNode(const std::string &name,
                                             const Vector3f &translate = Vector3f(0, 0, 0),
                                             const Quaternionf &rotate = Quaternionf(1, 0, 0, 0));

    virtual SceneNode *CreateRendererNode(const std::string &name,
                                           const std::string& filePath,
                                    const Vector3f &translate = Vector3f(0, 0, 0),
                                    const Quaternionf &rotate = Quaternionf(1, 0, 0, 0),
                                           const Vector3f &scale = Vector3f(1, 1, 1));

    void AddSceneNode(SceneNode *pNode,
               const Vector3f &translate = Vector3f(0, 0, 0),
               const Quaternionf &rotate = Quaternionf(1, 0, 0, 0),
                      const Vector3f &scale = Vector3f(1, 1, 1));

    virtual void AttachObject(SceneObject *obj);

    virtual void DetachAllObjects(void);

    virtual void DetachObject(SceneObject *obj);

    virtual SceneObject *DetachObject(uint32_t index);

    const std::vector<SceneObject*>& GetAllAttachedObjects() const;
    
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
    
    virtual void Update(float deltaTime);
    
    // 获得所有的子节点列表
    const std::vector<SceneNode*>& GetAllNodes() const;
    
private:
    std::string mName;                      //节点名称
    std::vector<SceneNode*> mChildNodes;    //孩子节点
    SceneNode* mParentNode = nullptr;       //父亲节点
    std::vector<SceneObject*> mAttachedObjects;   //该节点关联的物体
    std::vector<Component*> mComponents;        //组件列表
};


NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SCENE_NODE_INCLUDE_H */
