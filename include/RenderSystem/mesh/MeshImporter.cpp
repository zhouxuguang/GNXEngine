//
//  MeshImporter.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#include "MeshImporter.h"
#include "MeshAssimpImpoter.h"

NS_RENDERSYSTEM_BEGIN

MeshImporter::MeshImporter()
{
}

MeshImporter::~MeshImporter()
{
}

MeshImporter* CreateMeshImporter()
{
    return new(std::nothrow) MeshAssimpImpoter();
}

void DestroyMeshImporter(MeshImporter* meshImporter)
{
    delete meshImporter;
}

NS_RENDERSYSTEM_END
