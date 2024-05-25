//
//  SkinnedMeshRenderer.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/18.
//

#include "SkinnedMeshRenderer.h"
#include "BaseLib/LogService.h"

NS_RENDERSYSTEM_BEGIN

SkinnedMeshRenderer::SkinnedMeshRenderer()
{
}

SkinnedMeshRenderer::~SkinnedMeshRenderer()
{
}

void SkinnedMeshRenderer::SetSharedMesh(SkinnedMeshPtr mesh)
{
    mMeshPtr = mesh;
}

SkinnedMeshPtr SkinnedMeshRenderer::GetSharedMesh()
{
    return mMeshPtr;
}

void SkinnedMeshRenderer::AddMaterial(const MaterialPtr& material)
{
    mMaterials.push_back(material);
}

void SkinnedMeshRenderer::Update(float deltaTime)
{
    
}

void SkinnedMeshRenderer::Render(RenderInfo &renderInfo, bool isCPUSkin)
{
    if (mMaterials.empty())
    {
        log_info("MeshRenderer 材质列表为空！！！");
    }
    
    if (!mMeshPtr)
    {
        log_info("MeshRenderer mMeshPtr == nullptr！！！");
    }
    
    if (mMaterials.size() == 1)
    {
        renderInfo.materials = mMaterials;
        renderInfo.skinnedMatrixUBO = mMeshPtr->GetSkinnedMatrixBuffer();
        MeshDrawUtil::DrawSkinnedMesh(*mMeshPtr, renderInfo, isCPUSkin);
    }
    
    else
    {
        // 多个材质，那就必须要和子网格的数量保持一致
        renderInfo.materials = mMaterials;
        renderInfo.skinnedMatrixUBO = mMeshPtr->GetSkinnedMatrixBuffer();
        MeshDrawUtil::DrawSkinnedMesh(*mMeshPtr, renderInfo, isCPUSkin);
    }
}

NS_RENDERSYSTEM_END
