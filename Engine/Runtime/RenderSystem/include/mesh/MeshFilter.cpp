//
//  MeshFilter.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#include "MeshFilter.h"

NS_RENDERSYSTEM_BEGIN

MeshFilter::MeshFilter()
{
    //
}

MeshFilter::~MeshFilter()
{
    //
}

void MeshFilter::SetSharedMesh(MeshPtr mesh)
{
    mMeshPtr = mesh;
}

MeshPtr MeshFilter::GetSharedMesh()
{
    return mMeshPtr;
}

void MeshFilter::AssignMeshToRenderer()
{
    //
}

NS_RENDERSYSTEM_END
