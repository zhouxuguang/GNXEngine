//
//  MeshDrawUtil.h
//  rendersystem
//
//  Created by zhouxuguang on 2024/5/3.
//

#ifndef GNXENGINE_MESH_DRAW_UTIL_INCLUDE_HHGFHDFGKJ
#define GNXENGINE_MESH_DRAW_UTIL_INCLUDE_HHGFHDFGKJ

#include "Runtime/RenderSystem/include/RSDefine.h"
#include "Runtime/RenderSystem/include/Material.h"
#include "../Component.h"
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
    /**
     * 完整渲染（包含所有顶点属性）
     */
    static void DrawMesh(const Mesh& mesh, const RenderInfo& renderInfo);
    
    /**
     * 深度渲染（只渲染位置数据，不拷贝数据）
     * 
     * 优化策略：
     * 1. 只绑定位置属性到顶点缓冲区
     * 2. 使用简化的深度PSO
     * 3. 减少顶点着色器的工作量
     * 
     * @param mesh 要渲染的网格
     * @param renderInfo 渲染信息
     * @param depthPSO 深度渲染的PSO（只有位置输入）
     */
    static void DrawMeshDepthOnly(const Mesh& mesh, const RenderInfo& renderInfo, GraphicsPipelinePtr depthPSO);
    
    /**
     * 蒙皮网格完整渲染
     */
    static void DrawSkinnedMesh(const SkinnedMesh& mesh, const RenderInfo& renderInfo, bool isCPUSkin);
    
    /**
     * 深度渲染（蒙皮网格）
     *
     * @param mesh 蒙皮网格
     * @param renderInfo 渲染信息
     * @param depthPSO 深度渲染的PSO（位置+骨骼索引+权重）
     */
    static void DrawSkinnedMeshDepthOnly(const SkinnedMesh& mesh, const RenderInfo& renderInfo, GraphicsPipelinePtr depthPSO);
    
private:
    /**
     * 检查网格是否包含指定通道
     */
    static bool HasChannel(const Mesh& mesh, ShaderChannel channel);
    
    /**
     * 检查蒙皮网格是否包含指定通道
     */
    static bool HasChannel(const SkinnedMesh& mesh, ShaderChannel channel);
};

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_MESH_DRAW_UTIL_INCLUDE_HHGFHDFGKJ */
