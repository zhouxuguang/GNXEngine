//
//  SkinnedMeshRenderer.h
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/18.
//

#ifndef GNX_ENGINE_SKINNED_MESH_RENDERER_INCLUDE_HDFHJJJD
#define GNX_ENGINE_SKINNED_MESH_RENDERER_INCLUDE_HDFHJJJD

#include "../RSDefine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "../Component.h"
#include "SkinnedMesh.h"
#include "../Material.h"
#include "../mesh/MeshDrawUtil.h"

NS_RENDERSYSTEM_BEGIN

// 蒙皮网格渲染器
class SkinnedMeshRenderer : public Component
{
public:
    SkinnedMeshRenderer();
    
    ~SkinnedMeshRenderer();

    ComponentType GetComponentType() const override { return ComponentType::SkinnedRenderer; }
    
    void SetSharedMesh(SkinnedMeshPtr mesh);
    SkinnedMeshPtr GetSharedMesh();
    
    void AddMaterial(const MaterialPtr& material);
    
    const std::vector<MaterialPtr>& GetMaterials() const
    {
        return mMaterials;
    }
    
    void Update(float deltaTime) override;
    
    void Render(RenderInfo &renderInfo, bool isCPUSkin);

    // 是否投射阴影（默认 true）。接收阴影的网格应设为 false，
    // 避免其作为 caster 画进 ShadowMap 造成自阴影痤疮。
    void SetCastShadow(bool cast) { mCastShadow = cast; }
    bool GetCastShadow() const { return mCastShadow; }

private:
    SkinnedMeshPtr mMeshPtr = nullptr;
    typedef std::vector<MaterialPtr> MaterialPtrVector;
    MaterialPtrVector mMaterials;

    bool mCastShadow = true;   // 是否投射阴影（默认投射）
};

typedef std::shared_ptr<SkinnedMeshRenderer> SkinnedMeshRendererPtr;

template<> struct ComponentTypeOf<SkinnedMeshRenderer>
{
    static constexpr ComponentType Value = ComponentType::SkinnedRenderer;
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SKINNED_MESH_RENDERER_INCLUDE_HDFHJJJD */
