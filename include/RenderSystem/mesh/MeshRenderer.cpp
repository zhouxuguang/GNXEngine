//
//  MeshRenderer.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MeshRenderer.h"
#include "BaseLib/LogService.h"

NS_RENDERSYSTEM_BEGIN

MeshRenderer::MeshRenderer()
{
    //
}

MeshRenderer::~MeshRenderer()
{
    //
}

void MeshRenderer::SetSharedMesh(MeshPtr mesh)
{
    mMeshPtr = mesh;
}

MeshPtr MeshRenderer::GetSharedMesh()
{
    return mMeshPtr;
}

void MeshRenderer::AddMaterial(const MaterialPtr& material)
{
    mMaterials.push_back(material);
}

void MeshRenderer::Render(RenderInfo& renderInfo)
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
        MeshDrawUtil::DrawMesh(*mMeshPtr, renderInfo);
        //
    }
    
    else
    {
        // 多个材质，那就必须要和子网格的数量保持一致
        renderInfo.materials = mMaterials;
        MeshDrawUtil::DrawMesh(*mMeshPtr, renderInfo);
    }
}

NS_RENDERSYSTEM_END
