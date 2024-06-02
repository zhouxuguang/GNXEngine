//
//  MTLGraphicsPipeline.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#ifndef GNX_ENGINE_MTL_GRAPGICS_PIPELINE_INCLUDE_GDSF_GH
#define GNX_ENGINE_MTL_GRAPGICS_PIPELINE_INCLUDE_GDSF_GH

#include "MTLRenderDefine.h"
#include "GraphicsPipeline.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLGraphicsPipeline : public GraphicsPipeline
{
public:
    MTLGraphicsPipeline(id<MTLDevice> device, const GraphicsPipelineDescriptor& des);
    
    ~MTLGraphicsPipeline();
    
    virtual void attachVertexShader(ShaderFunctionPtr shaderFunction);
    
    virtual void attachFragmentShader(ShaderFunctionPtr shaderFunction);
    
    void Generate(const FrameBufferFormat& frameBufferFormat);
    
    id<MTLRenderPipelineState> getRenderPipelineState() const;
    
    id<MTLDepthStencilState> GetDepthStencilState() const
    {
        return mDepthStencilState;
    }
    
    MTLRenderPipelineReflection* GetReflectionObject() const
    {
        return mReflectionObj;
    }
    
    uint32_t GetVertexUniformOffset() const
    {
        return mVertexUniformOffset;
    }
    
private:
    id<MTLDevice> mDevice = nil;
    id<MTLRenderPipelineState> mRenderPipelineState = nil;
    MTLRenderPipelineDescriptor* mRenderPipelineDes = nil;
    id<MTLDepthStencilState> mDepthStencilState = nil;
    MTLRenderPipelineReflection* mReflectionObj = nil;
    
    uint32_t mVertexUniformOffset = 0;
    
    bool mGenerated = false;
};

typedef std::shared_ptr<MTLGraphicsPipeline> MTLGraphicsPipelinePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_GRAPGICS_PIPELINE_INCLUDE_GDSF_GH */
