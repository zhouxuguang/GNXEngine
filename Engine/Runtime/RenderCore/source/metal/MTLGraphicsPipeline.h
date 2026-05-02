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
#include "MTLShaderFunction.h"
#include <unordered_map>

NAMESPACE_RENDERCORE_BEGIN

// Forward declaration (to avoid circular include)
class MTLPipelineCache;

class MTLGraphicsPipeline : public GraphicsPipeline
{
public:
    MTLGraphicsPipeline(id<MTLDevice> device, const GraphicsPipelineDesc& des,
                        const std::shared_ptr<MTLPipelineCache>& pipelineCache = nullptr);
    
    ~MTLGraphicsPipeline();
    
    virtual void AttachVertexShader(ShaderFunctionPtr shaderFunction);
    
    virtual void AttachFragmentShader(ShaderFunctionPtr shaderFunction);
    
    virtual void AttachGraphicsShader(GraphicsShaderPtr graphicsShader);
    
    virtual void AttachTaskShader(ShaderFunctionPtr shaderFunction) override;
    virtual void AttachMeshShader(ShaderFunctionPtr shaderFunction) override;
    
    void Generate(const FrameBufferFormat& frameBufferFormat);
    
    id<MTLRenderPipelineState> getRenderPipelineState() const;
    
    /**
     * @brief 获取 Mesh Pipeline State（仅 Mesh 模式有效）
     */
    id<MTLRenderPipelineState> getMeshPipelineState() const { return mMeshPipelineState; }
    
    bool IsMeshPipeline() const { return mMeshPipelineState != nil; }

    // Mesh Pipeline PSO 反射属性：threadgroup 大小（从 Metal 编译后的 PSO 获取）
    const uint32_t* GetMeshThreadgroupSize() const { return mMeshThreadgroupSize; }
    const uint32_t* GetTaskThreadgroupSize() const { return mTaskThreadgroupSize; }

    id<MTLDepthStencilState> GetDepthStencilState() const
    {
        return mDepthStencilState;
    }

//    MTLRenderPipelineReflection* GetReflectionObject()
//    {
//        return mReflectionObj;
//    }

    // Mesh/Task shader resource bindings (populated from pipeline reflection)
    NSUInteger GetMeshResourceBindIndex(const std::string& resourceName) const
    {
        auto iter = mMeshBindings.find(resourceName);
        if (iter != mMeshBindings.end())
            return iter->second;
        return InvalidBindingIndex;
    }

    NSUInteger GetTaskResourceBindIndex(const std::string& resourceName) const
    {
        auto iter = mTaskBindings.find(resourceName);
        if (iter != mTaskBindings.end())
            return iter->second;
        return InvalidBindingIndex;
    }
    
//    MTLRenderPipelineReflection* GetReflectionObject() const
//    {
//        return mReflectionObj;
//    }
    
    uint32_t GetVertexUniformOffset() const
    {
        return mVertexUniformOffset;
    }
    
    MTLGraphicsShaderPtr GetShader() const;
    
private:
    id<MTLDevice> mDevice = nil;
    id<MTLRenderPipelineState> mRenderPipelineState = nil;
    MTLRenderPipelineDescriptor* mRenderPipelineDes = nil;
    id<MTLDepthStencilState> mDepthStencilState = nil;
    
    // Mesh Pipeline 相关（与传统 Pipeline 互斥）
    MTLMeshRenderPipelineDescriptor* mMeshPipelineDes = nil;
    id<MTLRenderPipelineState> mMeshPipelineState = nil;
    
    uint32_t mVertexUniformOffset = 0;

    MTLGraphicsShaderPtr mShader = nullptr;

    // Mesh/Task shader resource bindings (populated from pipeline reflection)
    std::unordered_map<std::string, NSUInteger> mMeshBindings;
    std::unordered_map<std::string, NSUInteger> mTaskBindings;

    // Mesh Pipeline PSO 反射的 threadgroup 大小（在 Generate() 中从 PSO 读取）
    uint32_t mMeshThreadgroupSize[3] = {1, 1, 1};
    uint32_t mTaskThreadgroupSize[3] = {1, 1, 1};
    
    std::shared_ptr<MTLPipelineCache> mPipelineCache;
    
    bool mGenerated = false;
};

typedef std::shared_ptr<MTLGraphicsPipeline> MTLGraphicsPipelinePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_GRAPGICS_PIPELINE_INCLUDE_GDSF_GH */
