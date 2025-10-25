//
//  SkinnedMeshRenderer.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/18.
//

#ifndef GNX_ENGINE_SKINNED_MESH_RENDERER_INCLUDE_HDFHJJJD
#define GNX_ENGINE_SKINNED_MESH_RENDERER_INCLUDE_HDFHJJJD

#include "RenderSystem/RSDefine.h"
#include "RenderCore/RenderDevice.h"
#include "Component.h"
#include "SkinnedMesh.h"
#include "Material.h"
#include "../mesh/MeshDrawUtil.h"

NS_RENDERSYSTEM_BEGIN

// 蒙皮网格渲染器
class SkinnedMeshRenderer : public Component
{
public:
    SkinnedMeshRenderer();
    
    ~SkinnedMeshRenderer();
    
    void SetSharedMesh(SkinnedMeshPtr mesh);
    SkinnedMeshPtr GetSharedMesh();
    
    void AddMaterial(const MaterialPtr& material);
    
    void Update(float deltaTime);
    
    void Render(RenderInfo &renderInfo, bool isCPUSkin);
private:
    SkinnedMeshPtr mMeshPtr = nullptr;
    typedef std::vector<MaterialPtr> MaterialPtrVector;
    MaterialPtrVector mMaterials;
};

typedef std::shared_ptr<SkinnedMeshRenderer> SkinnedMeshRendererPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SKINNED_MESH_RENDERER_INCLUDE_HDFHJJJD */
