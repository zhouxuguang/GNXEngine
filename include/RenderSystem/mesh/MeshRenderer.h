//
//  MeshRenderer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNXENGINE_MESH_RENDERER_INCLUDE_JSDJ
#define GNXENGINE_MESH_RENDERER_INCLUDE_JSDJ

#include "RenderSystem/RSDefine.h"
#include "RenderSystem/Material.h"
#include "Component.h"
#include "Mesh.h"
#include "MeshDrawUtil.h"

NS_RENDERSYSTEM_BEGIN

//主要负责静态模型的渲染工作
class MeshRenderer : public Component
{
public:
    MeshRenderer();
    
    ~MeshRenderer();
    
    void SetSharedMesh(MeshPtr mesh);
    MeshPtr GetSharedMesh();
    
    void AddMaterial(const MaterialPtr& material);
    
    void Render(RenderInfo &renderInfo);
private:
    MeshPtr mMeshPtr = nullptr;
    
    typedef std::vector<MaterialPtr> MaterialPtrVector;
    MaterialPtrVector mMaterials;
};

typedef std::shared_ptr<MeshRenderer> MeshRendererPtr;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_MESH_RENDERER_INCLUDE_JSDJ */
