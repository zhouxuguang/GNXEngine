//
//  MeshDrawUtil.h
//  rendersystem
//
//  Created by zhouxuguang on 2024/5/3.
//

#ifndef GNXENGINE_MESH_DRAW_UTIL_INCLUDE_HHGFHDFGKJ
#define GNXENGINE_MESH_DRAW_UTIL_INCLUDE_HHGFHDFGKJ

#include "RenderSystem/RSDefine.h"
#include "RenderSystem/Material.h"
#include "Component.h"
#include "Mesh.h"
#include "../skinnedMesh/SkinnedMesh.h"

NS_RENDERSYSTEM_BEGIN

//渲染的全局信息
struct RenderInfo
{
    UniformBufferPtr cameraUBO;   //相机的ubo信息
    UniformBufferPtr objectUBO;   //物体信息
    UniformBufferPtr lightUBO;    //灯光信息
    RenderEncoderPtr renderEncoder;           //当前的渲染pass
    std::vector<MaterialPtr> materials;                 //当前的材质
    UniformBufferPtr skinnedMatrixUBO;   //GPU蒙皮矩阵
};

class MeshDrawUtil
{
public:
    static void DrawMesh(const Mesh& mesh, const RenderInfo& renderInfo);
    
    static void DrawSkinnedMesh(const SkinnedMesh& mesh, const RenderInfo& renderInfo, bool isCPUSkin);
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_MESH_DRAW_UTIL_INCLUDE_HHGFHDFGKJ */
