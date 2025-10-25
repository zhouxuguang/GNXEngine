//
//  MeshFilter.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef GNX_ENGINE_MESH_FILTER_INCLUDESDFD_H
#define GNX_ENGINE_MESH_FILTER_INCLUDESDFD_H

#include "RenderSystem/RSDefine.h"
#include "Mesh.h"
#include "Component.h"

NS_RENDERSYSTEM_BEGIN

class MeshFilter : public Component
{
public:
    MeshFilter();
    
    ~MeshFilter();
    
    void SetSharedMesh(MeshPtr mesh);
    MeshPtr GetSharedMesh();
    
private:
    MeshPtr mMeshPtr = nullptr;
    void AssignMeshToRenderer();
};

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_MESH_FILTER_INCLUDESDFD_H */
