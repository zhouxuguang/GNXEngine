//
//  VKRenderEncoder.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#ifndef GNX_ENGINE_VK_RENDER_ENCODER_INCLUDEKDSG
#define GNX_ENGINE_VK_RENDER_ENCODER_INCLUDEKDSG

#include "VulkanContext.h"
#include "RenderEncoder.h"

NAMESPACE_RENDERCORE_BEGIN

class VKRenderEncoder : public RenderEncoder
{
public:
    VKRenderEncoder(VkCommandBuffer commandBuffer, const VkRenderingInfoKHR& renderInfo);
    
    ~VKRenderEncoder();
    
    virtual void EndEncode();
    
    virtual void setGraphicsPipeline(GraphicsPipelinePtr graphicsPipeline){}
    
    virtual void setVertexBuffer(VertexBufferPtr buffer, uint32_t offset, int index){}
    
    virtual void setVertexBytes(const void* pData, size_t dataLen, int index){}
    
    virtual void setVertexUniformBuffer(UniformBufferPtr buffer, int index){}
    
    virtual void setFragmentUniformBuffer(UniformBufferPtr buffer, int index){}
    
    virtual void drawPrimitves(PrimitiveMode mode, int offset, int size){}
    
    virtual void drawIndexedPrimitives(PrimitiveMode mode, int size, IndexBufferPtr buffer, int offset){}
    
    virtual void setFragmentTextureAndSampler(Texture2DPtr texture, TextureSamplerPtr sampler, int index){}
    
    virtual void setFragmentTextureCubeAndSampler(TextureCubePtr textureCube, TextureSamplerPtr sampler, int index){}
    
    virtual void setFragmentRenderTextureAndSampler(RenderTexturePtr textureCube, TextureSamplerPtr sampler, int index){}
    
private:
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
};

using VKRenderEncoderPtr = std::shared_ptr<VKRenderEncoder>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_RENDER_ENCODER_INCLUDEKDSG */
