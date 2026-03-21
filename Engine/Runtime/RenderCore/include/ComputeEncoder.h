//
//  ComputeEncoder.h
//  GNXEngine
//
//  Created by zhouxuguang on 2024/5/12.
//

#ifndef GNX_ENGINE_COMPUTE_ENCODER_HINFKSG
#define GNX_ENGINE_COMPUTE_ENCODER_HINFKSG

#include "RenderDefine.h"
#include "GraphicsPipeline.h"
#include "RCBuffer.h"
#include "RCTexture.h"
#include "UniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

// 计算着色器的encoder
class ComputeEncoder
{
public:
    ComputeEncoder()
    {
    }
    
    virtual ~ComputeEncoder()
    {
    }
    
    virtual void SetComputePipeline(ComputePipelinePtr computePipeline) = 0;
    
    virtual void SetUniformBuffer(const std::string& resourceName, UniformBufferPtr buffer) = 0;
    
    /**
     * @brief 设置RCBuffer作为存储缓冲区（SSBO）
     * @param buffer RCBuffer指针
     * @param index 绑定索引
     */
    virtual void SetStorageBuffer(RCBufferPtr buffer, uint32_t index) = 0;
    
    /**
     * @brief 通过索引设置纹理（用于采样）
     * @param texture 纹理指针
     * @param index 绑定索引
     */
    virtual void SetTexture(RCTexturePtr texture, uint32_t index) = 0;
    
    /**
     * @brief 通过索引设置纹理（指定Mip层级，用于采样）
     * @param texture 纹理指针
     * @param mipLevel Mip层级
     * @param index 绑定索引
     */
    virtual void SetTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) = 0;
    
    /**
     * @brief 通过资源名设置纹理（用于采样）- 推荐使用
     * @param resourceName Shader中的资源名（如 "srcDepth"）
     * @param texture 纹理指针
     */
    virtual void SetTexture(const std::string& resourceName, RCTexturePtr texture) = 0;
    
    /**
     * @brief 通过资源名设置纹理（指定Mip层级）- 推荐使用
     * @param resourceName Shader中的资源名
     * @param texture 纹理指针
     * @param mipLevel Mip层级
     */
    virtual void SetTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) = 0;
    
    /**
     * @brief 通过索引设置输出纹理（用于写入，UAV/Storage Image）
     * @param texture 纹理指针
     * @param index 绑定索引
     */
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t index) = 0;
    
    /**
     * @brief 通过索引设置输出纹理（指定Mip层级）
     * @param texture 纹理指针
     * @param mipLevel Mip层级
     * @param index 绑定索引
     */
    virtual void SetOutTexture(RCTexturePtr texture, uint32_t mipLevel, uint32_t index) = 0;
    
    /**
     * @brief 通过资源名设置输出纹理 - 推荐使用
     * @param resourceName Shader中的资源名（如 "dstDepth"）
     * @param texture 纹理指针
     */
    virtual void SetOutTexture(const std::string& resourceName, RCTexturePtr texture) = 0;
    
    /**
     * @brief 通过资源名设置输出纹理（指定Mip层级）- 推荐使用
     * @param resourceName Shader中的资源名
     * @param texture 纹理指针
     * @param mipLevel Mip层级
     */
    virtual void SetOutTexture(const std::string& resourceName, RCTexturePtr texture, uint32_t mipLevel) = 0;
    
    virtual void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ) = 0;
    
    virtual void EndEncode() = 0;
};

using ComputeEncoderPtr = std::shared_ptr<ComputeEncoder>;


NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_COMPUTE_ENCODER_HINFKSG */
