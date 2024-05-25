//
//  TestMesh.hpp
//  testNX
//
//  Created by zhouxuguang on 2022/5/28.
//

#ifndef TestMesh_hpp
#define TestMesh_hpp

#include <stdio.h>
#include "RenderSystem/mesh/Mesh.h"

using namespace RenderSystem;

void initMesh(rendercore::RenderDevicePtr renderDevice);

void testMesh(const rendercore::RenderEncoderPtr &renderEncoder);

#endif /* TestMesh_hpp */
