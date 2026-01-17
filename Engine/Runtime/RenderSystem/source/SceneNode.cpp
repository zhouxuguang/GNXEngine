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
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "RenderParameter.h"

NS_RENDERSYSTEM_BEGIN

SceneNode::SceneNode()
{
    mIsVisible = true;
    mIsActive = true;
    mWorldTransformDirty = true;
    mCachedWorldMatrix = mathutil::Matrix4x4f();
    mCachedModelUBO = nullptr;
    mModelUBODirty = true;
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

SceneNode * SceneNode::CreateChildSceneNode(const std::string &name, const Vector3f &translate, const Quaternionf &rotate)
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

SceneNode * SceneNode::CreateRendererNode(const std::string &name,
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

    // 检查是否已有 TransformComponent，避免重复添加
    TransformComponent* transform = pNode->QueryComponentT<TransformComponent>();
    if (transform)
    {
        // 已有 TransformComponent，更新变换
        transform->transform.position = translate;
        transform->transform.rotation = rotate;
        transform->transform.scale = scale;
        pNode->MarkWorldTransformDirty();
    }
    else
    {
        // 没有 TransformComponent，创建新的
        transform = new TransformComponent(Transform(translate, rotate, scale));
        pNode->AddComponent(transform);
        pNode->MarkWorldTransformDirty();
    }

    mChildNodes.push_back(pNode);
}

void SceneNode::AttachObject(SceneObject *obj)
{
    mAttachedObjects.push_back(obj);
}

void SceneNode::DetachAllObjects(void)
{
    mAttachedObjects.clear();
}

void SceneNode::DetachObject(SceneObject *obj)
{
    for (auto it = mAttachedObjects.begin(); it != mAttachedObjects.end(); ++it)
    {
        if (*it == obj)
        {
            mAttachedObjects.erase(it);
            break;
        }
    }
}

SceneObject * SceneNode::DetachObject(uint32_t index)
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

const std::vector<SceneObject*> &SceneNode::GetAllAttachedObjects() const
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

SceneNode* SceneNode::FindChild(const std::string& name) const
{
    for (SceneNode* child : mChildNodes)
    {
        if (child && child->GetName() == name)
        {
            return child;
        }
    }
    return nullptr;
}

SceneNode* SceneNode::FindChildRecursive(const std::string& name) const
{
    // 先在直接子节点中查找
    SceneNode* found = FindChild(name);
    if (found)
    {
        return found;
    }

    // 递归在子节点的子节点中查找
    for (SceneNode* child : mChildNodes)
    {
        if (child)
        {
            found = child->FindChildRecursive(name);
            if (found)
            {
                return found;
            }
        }
    }

    return nullptr;
}

std::vector<SceneNode*> SceneNode::GetAllDescendants() const
{
    std::vector<SceneNode*> descendants;

    for (SceneNode* child : mChildNodes)
    {
        if (child)
        {
            descendants.push_back(child);
            // 递归获取子节点的所有子孙节点
            std::vector<SceneNode*> childDescendants = child->GetAllDescendants();
            descendants.insert(descendants.end(), childDescendants.begin(), childDescendants.end());
        }
    }

    return descendants;
}

void SceneNode::RemoveChild(SceneNode* child)
{
    if (!child)
    {
        return;
    }

    auto it = std::find(mChildNodes.begin(), mChildNodes.end(), child);
    if (it != mChildNodes.end())
    {
        mChildNodes.erase(it);
        if (child->mParentNode == this)
        {
            child->mParentNode = nullptr;
        }
    }
}

void SceneNode::DestroyChild(SceneNode* child)
{
    if (!child)
    {
        return;
    }

    RemoveChild(child);
    delete child;
}

int SceneNode::GetDepth() const
{
    int depth = 0;
    const SceneNode* current = mParentNode;
    while (current)
    {
        depth++;
        current = current->mParentNode;
    }
    return depth;
}

void SceneNode::MarkWorldTransformDirty()
{
    mWorldTransformDirty = true;
    mModelUBODirty = true;  // 世界变换改变，UBO 也需要更新
    for (SceneNode* child : mChildNodes)
    {
        child->MarkWorldTransformDirty();
    }
}

mathutil::Matrix4x4f SceneNode::GetWorldTransform() const
{
    TransformComponent* transformCom = QueryComponentT<TransformComponent>();
    if (transformCom)
    {
        if (mWorldTransformDirty)
        {
            if (mParentNode)
            {
                mCachedWorldMatrix = mParentNode->GetWorldTransform() * transformCom->transform.TransformToMat4();
            }
            else
            {
                mCachedWorldMatrix = transformCom->transform.TransformToMat4();
            }
            mWorldTransformDirty = false;
            mModelUBODirty = true;  // 世界变换改变，标记 UBO 需要更新
        }
    }
    else
    {
        if (mWorldTransformDirty)
        {
            if (mParentNode)
            {
                mCachedWorldMatrix = mParentNode->GetWorldTransform();
            }
            else
            {
                mCachedWorldMatrix = mathutil::Matrix4x4f();
            }
            mWorldTransformDirty = false;
            mModelUBODirty = true;  // 世界变换改变，标记 UBO 需要更新
        }
    }
    return mCachedWorldMatrix;
}

void SceneNode::RemoveComponent(Component* component)
{
    if (!component)
    {
        return;
    }

    auto it = std::find(mComponents.begin(), mComponents.end(), component);
    if (it != mComponents.end())
    {
        mComponents.erase(it);
    }
}

void SceneNode::DestroyComponent(Component* component)
{
    if (!component)
    {
        return;
    }

    RemoveComponent(component);
    delete component;
}

UniformBufferPtr SceneNode::GetOrCreateModelUBO(RenderDevicePtr renderDevice)
{
    if (!mCachedModelUBO)
    {
        // 首次创建 UBO
        mCachedModelUBO = renderDevice->CreateUniformBufferWithSize(sizeof(cbPerObject));
    }

    if (mModelUBODirty && mCachedModelUBO)
    {
        // 更新 UBO 数据
        mathutil::Matrix4x4f worldMatrix = GetWorldTransform();
        cbPerObject modelMatrix;
        modelMatrix.MATRIX_M = worldMatrix;
        modelMatrix.MATRIX_M_INV = worldMatrix.Inverse();
        modelMatrix.MATRIX_Normal = worldMatrix.Transpose();
        mCachedModelUBO->SetData(&modelMatrix, 0, sizeof(cbPerObject));
        mModelUBODirty = false;
    }

    return mCachedModelUBO;
}

void SceneNode::MarkModelUBODirty()
{
    mModelUBODirty = true;
}

NS_RENDERSYSTEM_END
