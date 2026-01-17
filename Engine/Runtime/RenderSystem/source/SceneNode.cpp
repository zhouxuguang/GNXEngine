//
//  SceneNode.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/7/29.
//

#include "SceneNode.h"
#include "BuildSetting.h"
#include "mesh/MeshAssimpImpoter.h"
#include "RenderEngine.h"
#include "mesh/MeshRenderer.h"
#include "skinnedMesh/SkinnedMesh.h"
#include "skinnedMesh/SkinnedMeshRenderer.h"
#include "animation/Skeleton.h"
#include "animation/SkeletonAnimation.h"

NS_RENDERSYSTEM_BEGIN

SceneNode::SceneNode()
{
    //
}

SceneNode::~SceneNode()
{
    // 递归释放所有子节点
    for (SceneNode* child : mChildNodes)
    {
        delete child;
    }
    mChildNodes.clear();

    // 释放所有组件
    for (Component* comp : mComponents)
    {
        delete comp;
    }
    mComponents.clear();

    // 释放所有附加对象
    for (SceneObject* obj : mAttachedObjects)
    {
        delete obj;
    }
    mAttachedObjects.clear();
}

SceneNode * SceneNode::createChildSceneNode(const std::string &name, const Vector3f &translate, const Quaternionf &rotate)
{
    SceneNode *pNode = new SceneNode();
    pNode->SetName(name);
    pNode->mParentNode = this;

    TransformComponent* transform = new TransformComponent(
                       Transform(translate, rotate, Vector3f(1.0f, 1.0f, 1.0f)));
    pNode->AddComponent(transform);

    mChildNodes.push_back(pNode);
    return pNode;
}

SceneNode * SceneNode::createRendererNode(const std::string &name,
                                       const std::string& filePath,
                                const Vector3f &translate,
                                const Quaternionf &rotate,
                                          const Vector3f &scale)
{
    SceneNode *pNode = new SceneNode();
    pNode->SetName(name);
    pNode->mParentNode = this;
    
    TransformComponent* transform = new TransformComponent(
                       Transform(translate, rotate, scale));
    pNode->AddComponent(transform);
    
    //加载模型
    std::string modelPath = getMediaDir() + filePath;
    MeshPtr mesh = std::make_shared<Mesh>();
    SkinnedMeshPtr skinnedMesh = std::make_shared<SkinnedMesh>();
    
    RenderSystem::MeshAssimpImpoter meshImporter;
    meshImporter.ImportFromFile(modelPath.c_str(), mesh.get(), skinnedMesh.get());
    
    //根据顶点个数判断是否有蒙皮网格
    if (mesh->GetVertexCount())
    {
        MeshRenderer* meshRender = new(std::nothrow) MeshRenderer();
        meshRender->SetSharedMesh(mesh);
        for (auto iter : meshImporter.GetMaterials())
        {
            meshRender->AddMaterial(iter);
        }
        pNode->AddComponent(meshRender);
    }
    else
    {
        SkinnedMeshRenderer* meshRender = new(std::nothrow) SkinnedMeshRenderer();
        meshRender->SetSharedMesh(skinnedMesh);
        for (auto iter : meshImporter.GetMaterials())
        {
            meshRender->AddMaterial(iter);
        }
        pNode->AddComponent(meshRender);
        
        // 创建骨骼
        SkeletonPtr skeleton = std::make_shared<Skeleton>();
        skeleton->Set(meshImporter.GetBindPose(), meshImporter.GetBoneNames());
        
        SkeletonAnimation* skeletonAnimation = new(std::nothrow) SkeletonAnimation();
        skeletonAnimation->mAnimationClips = meshImporter.GetAnimationClips();
        skeletonAnimation->mSkeleton = skeleton;
        skeletonAnimation->mCPUSkin = BuildSetting::mCPUSkinning;
        
        skeletonAnimation->mAnimatedPose = skeleton->GetBindPose();
        skeletonAnimation->mPosePalette.resize(skeleton->GetBindPose().Size());
        
        pNode->AddComponent(skeletonAnimation);
    }
    
    mChildNodes.push_back(pNode);
    return pNode;
}

void SceneNode::AddSceneNode(SceneNode *pNode,
           const Vector3f &translate,
           const Quaternionf &rotate,
                  const Vector3f &scale)
{
    if (!pNode)
    {
        return;
    }

    // 如果节点已经有父节点，先从旧的父节点中移除
    if (pNode->mParentNode && pNode->mParentNode != this)
    {
        auto& siblings = pNode->mParentNode->mChildNodes;
        auto it = std::find(siblings.begin(), siblings.end(), pNode);
        if (it != siblings.end())
        {
            siblings.erase(it);
        }
    }

    pNode->mParentNode = this;

    TransformComponent* transform = new TransformComponent(
                       Transform(translate, rotate, scale));
    pNode->AddComponent(transform);

    mChildNodes.push_back(pNode);
}

void SceneNode::attachObject(SceneObject *obj)
{
    mAttachedObjects.push_back(obj);
}

void SceneNode::detachAllObjects(void)
{
    mAttachedObjects.clear();
}

void SceneNode::detachObject(SceneObject *obj)
{
    for (auto iter = mAttachedObjects.begin(); iter != mAttachedObjects.end(); iter ++)
    {
        if (*iter == obj)
        {
            mAttachedObjects.erase(iter);
            break;  // erase 后迭代器失效，必须 break
        }
    }
}

SceneObject * SceneNode::detachObject(uint32_t index)
{
    if (index >= mAttachedObjects.size())
    {
        return nullptr;
    }

    auto iter = mAttachedObjects.begin() + index;
    SceneObject* obj = *iter;  // 先保存对象指针
    mAttachedObjects.erase(iter);

    return obj;  // 返回保存的对象指针
}

const std::vector<SceneObject*> &SceneNode::getAllAttachedObjects() const
{
    return mAttachedObjects;
}

Component* SceneNode::GetComponentPtrAtIndex(int i) const
{
    if (i < 0 || i >= mComponents.size())
    {
        return nullptr;
    }
    
    return mComponents[i];
}

void SceneNode::Update(float deltaTime)
{
    for (auto iter : mComponents)
    {
        iter->Update(deltaTime);
    }
}

const std::vector<SceneNode*>& SceneNode::GetAllNodes() const
{
    return mChildNodes;
}

NS_RENDERSYSTEM_END
