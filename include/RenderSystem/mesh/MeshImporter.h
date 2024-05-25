//
//  MeshImporter.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef FNXENGINE_MESH_IMPORTER_INCLUDE_H
#define FNXENGINE_MESH_IMPORTER_INCLUDE_H

#include "RenderSystem/RSDefine.h"
#include "Mesh.h"

NS_RENDERSYSTEM_BEGIN

class SkinnedMesh;

class MeshImporter
{
public:
    MeshImporter();
    
    ~MeshImporter();
    
    virtual bool ImportFromFile(const std::string &fileName, Mesh* mesh, SkinnedMesh* skinnedMesh) = 0;
};

NS_RENDERSYSTEM_END

#endif /* FNXENGINE_MESH_IMPORTER_INCLUDE_H */
