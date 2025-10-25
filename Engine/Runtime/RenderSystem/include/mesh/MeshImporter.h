//
//  MeshImporter.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef FNXENGINE_MESH_IMPORTER_INCLUDE_H
#define FNXENGINE_MESH_IMPORTER_INCLUDE_H

#include "RSDefine.h"
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

MeshImporter* CreateMeshImporter();

void DestroyMeshImporter(MeshImporter* meshImporter);

NS_RENDERSYSTEM_END

#endif /* FNXENGINE_MESH_IMPORTER_INCLUDE_H */
