//
//  MTLGraphicsPipeline.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#include "MTLGraphicsPipeline.h"
#include "MTLShaderFunction.h"
#include "MTLPipelineCache.h"

NAMESPACE_RENDERCORE_BEGIN

static id<MTLDepthStencilState> convertToMTLDSDes(id<MTLDevice> device, const DepthStencilDesc& depthStencilDescriptor)
{
    MTLDepthStencilDescriptor* mtlDescriptor = [MTLDepthStencilDescriptor new];
    MTLCompareFunction compareFunc = (MTLCompareFunction)depthStencilDescriptor.depthCompareFunction;
    mtlDescriptor.depthCompareFunction = compareFunc;
    mtlDescriptor.depthWriteEnabled = depthStencilDescriptor.depthWriteEnabled;
    
    
    MTLStencilDescriptor * stencilDescriptor = nil;
    if (!depthStencilDescriptor.stencil.stencilEnable)
    {
        mtlDescriptor.frontFaceStencil = nil;
        mtlDescriptor.backFaceStencil = nil;
    }
    else
    {
        stencilDescriptor = [MTLStencilDescriptor new];
        stencilDescriptor.stencilCompareFunction = (MTLCompareFunction)depthStencilDescriptor.stencil.stencilCompareFunction;
        stencilDescriptor.stencilFailureOperation = (MTLStencilOperation)depthStencilDescriptor.stencil.stencilFailureOperation;
        stencilDescriptor.depthFailureOperation = (MTLStencilOperation)depthStencilDescriptor.stencil.depthFailureOperation;
        stencilDescriptor.depthStencilPassOperation = (MTLStencilOperation)depthStencilDescriptor.stencil.depthStencilPassOperation;
        stencilDescriptor.readMask = depthStencilDescriptor.stencil.readMask;
        stencilDescriptor.writeMask = depthStencilDescriptor.stencil.writeMask;
        mtlDescriptor.frontFaceStencil = stencilDescriptor;
        mtlDescriptor.backFaceStencil = stencilDescriptor;
    }
    
    return [device newDepthStencilStateWithDescriptor:mtlDescriptor];
}

static MTLVertexFormat convertMETALVertexFormat(VertexFormat format)
{
    MTLVertexFormat mtlFormat = MTLVertexFormatInvalid;
    switch (format)
    {
        case VertexFormatUChar:
            mtlFormat = MTLVertexFormatUChar;
            break;
            
        case VertexFormatUChar2:
            mtlFormat = MTLVertexFormatUChar2;
            break;
            
        case VertexFormatUChar3:
            mtlFormat = MTLVertexFormatUChar3;
            break;
            
        case VertexFormatUChar4:
            mtlFormat = MTLVertexFormatUChar4;
            break;
            
        case VertexFormatChar:
            mtlFormat = MTLVertexFormatChar;
            break;
            
        case VertexFormatChar2:
            mtlFormat = MTLVertexFormatChar2;
            break;
            
        case VertexFormatChar3:
            mtlFormat = MTLVertexFormatChar3;
            break;
            
        case VertexFormatChar4:
            mtlFormat = MTLVertexFormatChar4;
            break;
            
        case VertexFormatUShort:
            mtlFormat = MTLVertexFormatUShort;
            break;
            
        case VertexFormatUShort2:
            mtlFormat = MTLVertexFormatUShort2;
            break;
            
        case VertexFormatUShort3:
            mtlFormat = MTLVertexFormatUShort3;
            break;
            
        case VertexFormatUShort4:
            mtlFormat = MTLVertexFormatUShort4;
            break;
            
        case VertexFormatShort:
            mtlFormat = MTLVertexFormatShort;
            break;
            
        case VertexFormatShort2:
            mtlFormat = MTLVertexFormatShort2;
            break;
            
        case VertexFormatShort3:
            mtlFormat = MTLVertexFormatShort3;
            break;
            
        case VertexFormatShort4:
            mtlFormat = MTLVertexFormatShort4;
            break;
            
        case VertexFormatFloat:
            mtlFormat = MTLVertexFormatFloat;
            break;
            
        case VertexFormatFloat2:
            mtlFormat = MTLVertexFormatFloat2;
            break;
            
        case VertexFormatFloat3:
            mtlFormat = MTLVertexFormatFloat3;
            break;
            
        case VertexFormatFloat4:
            mtlFormat = MTLVertexFormatFloat4;
            break;
            
        case VertexFormatInt:
            mtlFormat = MTLVertexFormatInt;
            break;
            
        case VertexFormatInt2:
            mtlFormat = MTLVertexFormatInt2;
            break;
            
        case VertexFormatInt3:
            mtlFormat = MTLVertexFormatInt3;
            break;
            
        case VertexFormatInt4:
            mtlFormat = MTLVertexFormatInt4;
            break;
            
        case VertexFormatUInt:
            mtlFormat = MTLVertexFormatUInt;
            break;
            
        case VertexFormatUInt2:
            mtlFormat = MTLVertexFormatUInt2;
            break;
            
        case VertexFormatUInt3:
            mtlFormat = MTLVertexFormatUInt3;
            break;
            
        case VertexFormatUInt4:
            mtlFormat = MTLVertexFormatUInt4;
            break;
            
        case VertexFormatHalfFloat:
            mtlFormat = MTLVertexFormatHalf;
            break;
            
        case VertexFormatHalfFloat2:
            mtlFormat = MTLVertexFormatHalf2;
            break;
            
        case VertexFormatHalfFloat3:
            mtlFormat = MTLVertexFormatHalf3;
            break;
            
        case VertexFormatHalfFloat4:
            mtlFormat = MTLVertexFormatHalf4;
            break;
            
        // ★★★ 无符号归一化格式: uint8 [0,255] → float [0,1] ★★★
        case VertexFormatUCharNorm:
            mtlFormat = MTLVertexFormatUCharNormalized;
            break;
            
        case VertexFormatUChar2Norm:
            mtlFormat = MTLVertexFormatUChar2Normalized;
            break;
            
        case VertexFormatUChar3Norm:
            mtlFormat = MTLVertexFormatUChar3Normalized;
            break;
            
        case VertexFormatUChar4Norm:
            mtlFormat = MTLVertexFormatUChar4Normalized;
            break;
            
        // ★★★ 有符号归一化格式: int8 [-128,127] → float [-1,1] ★★★
        case VertexFormatCharNorm:
            mtlFormat = MTLVertexFormatCharNormalized;
            break;
            
        case VertexFormatChar2Norm:
            mtlFormat = MTLVertexFormatChar2Normalized;
            break;
            
        case VertexFormatChar3Norm:
            mtlFormat = MTLVertexFormatChar3Normalized;   // 法线归一化常用
            break;
            
        case VertexFormatChar4Norm:
            mtlFormat = MTLVertexFormatChar4Normalized;
            break;
            
        default:
            break;
    }
    
    return mtlFormat;
}

static MTLRenderPipelineDescriptor* convertToMTLRenderPiplineDescriptor(const GraphicsPipelineDesc& des)
{
    // 设置shader
    MTLRenderPipelineDescriptor* mtlPiplineDes = [MTLRenderPipelineDescriptor new];
    mtlPiplineDes.vertexFunction = nil;
    mtlPiplineDes.fragmentFunction = nil;
    
    // 设置顶点属性
    std::vector<int> indexs;
    for (const auto& iter : des.vertexDescriptor.attributes)
    {
        mtlPiplineDes.vertexDescriptor.attributes[iter.index].bufferIndex = iter.index;
        mtlPiplineDes.vertexDescriptor.attributes[iter.index].format = convertMETALVertexFormat(iter.format);
        mtlPiplineDes.vertexDescriptor.attributes[iter.index].offset = iter.offset;
        indexs.push_back(iter.index);
    }
    
    for (size_t i = 0; i < des.vertexDescriptor.layouts.size(); i ++)
    {
        mtlPiplineDes.vertexDescriptor.layouts[indexs[i]].stride = des.vertexDescriptor.layouts[i].stride;
        mtlPiplineDes.vertexDescriptor.layouts[indexs[i]].stepRate = 1;
        mtlPiplineDes.vertexDescriptor.layouts[indexs[i]].stepFunction = MTLVertexStepFunctionPerVertex;
    }
    
    for (uint32_t i = 0; i < des.renderTargetCount; i ++)
    {
        mtlPiplineDes.colorAttachments[i].blendingEnabled = des.colorAttachmentDescriptors[i].blendingEnabled;
        mtlPiplineDes.colorAttachments[i].sourceRGBBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].sourceRGBBlendFactor;
        mtlPiplineDes.colorAttachments[i].destinationRGBBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].destinationRGBBlendFactor;
        mtlPiplineDes.colorAttachments[i].rgbBlendOperation = (MTLBlendOperation)des.colorAttachmentDescriptors[i].rgbBlendOperation;
        mtlPiplineDes.colorAttachments[i].sourceAlphaBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].sourceAlphaBlendFactor;
        mtlPiplineDes.colorAttachments[i].destinationAlphaBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].destinationAplhaBlendFactor;
        mtlPiplineDes.colorAttachments[i].alphaBlendOperation = (MTLBlendOperation)des.colorAttachmentDescriptors[i].aplhaBlendOperation;
        mtlPiplineDes.colorAttachments[i].writeMask = (MTLColorWriteMask)des.colorAttachmentDescriptors[i].writeMask;
    }
    
    return mtlPiplineDes;
}

MTLGraphicsPipeline::MTLGraphicsPipeline(id<MTLDevice> device, const GraphicsPipelineDesc& des,
                                           const std::shared_ptr<MTLPipelineCache>& pipelineCache) : GraphicsPipeline(des)
{
    mDevice = device;
    mDesc = des;
    mPipelineCache = pipelineCache;
    mDepthStencilState = convertToMTLDSDes(device, des.depthStencilDescriptor);
    
    if (des.pipelineType == PipelineType::Mesh)
    {
        // Mesh Pipeline 模式
        mMeshPipelineDes = [MTLMeshRenderPipelineDescriptor new];
        mMeshPipelineDes.objectFunction = nil;
        mMeshPipelineDes.meshFunction = nil;
        mMeshPipelineDes.fragmentFunction = nil;
        
        // 设置颜色附件
        for (uint32_t i = 0; i < des.renderTargetCount; i ++)
        {
            mMeshPipelineDes.colorAttachments[i].blendingEnabled = des.colorAttachmentDescriptors[i].blendingEnabled;
            mMeshPipelineDes.colorAttachments[i].sourceRGBBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].sourceRGBBlendFactor;
            mMeshPipelineDes.colorAttachments[i].destinationRGBBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].destinationRGBBlendFactor;
            mMeshPipelineDes.colorAttachments[i].rgbBlendOperation = (MTLBlendOperation)des.colorAttachmentDescriptors[i].rgbBlendOperation;
            mMeshPipelineDes.colorAttachments[i].sourceAlphaBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].sourceAlphaBlendFactor;
            mMeshPipelineDes.colorAttachments[i].destinationAlphaBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptors[i].destinationAplhaBlendFactor;
            mMeshPipelineDes.colorAttachments[i].alphaBlendOperation = (MTLBlendOperation)des.colorAttachmentDescriptors[i].aplhaBlendOperation;
            mMeshPipelineDes.colorAttachments[i].writeMask = (MTLColorWriteMask)des.colorAttachmentDescriptors[i].writeMask;
        }
    }
    else
    {
        // 传统 Graphics Pipeline 模式
        mRenderPipelineDes = convertToMTLRenderPiplineDescriptor(des);
    }
}

MTLGraphicsPipeline::~MTLGraphicsPipeline()
{
    //
}

void MTLGraphicsPipeline::AttachVertexShader(ShaderFunctionPtr shaderFunction)
{
    if (!shaderFunction)
    {
        return;
    }
    
    MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(shaderFunction);
    mRenderPipelineDes.vertexFunction = shaderPtr->GetShaderFunction();
}

void MTLGraphicsPipeline::AttachFragmentShader(ShaderFunctionPtr shaderFunction)
{
    if (!shaderFunction)
    {
        return;
    }
    
    MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(shaderFunction);

    if (mDesc.pipelineType == PipelineType::Mesh && mMeshPipelineDes)
    {
        mMeshPipelineDes.fragmentFunction = shaderPtr->GetShaderFunction();
    }
    else if (mRenderPipelineDes)
    {
        mRenderPipelineDes.fragmentFunction = shaderPtr->GetShaderFunction();
    }
}

void MTLGraphicsPipeline::AttachGraphicsShader(GraphicsShaderPtr graphicsShader)
{
    if (!graphicsShader)
    {
        return;
    }
    
    MTLGraphicsShaderPtr shaderPtr = std::dynamic_pointer_cast<MTLGraphicsShader>(graphicsShader);
    mShader = shaderPtr;
    
    if (mDesc.pipelineType == PipelineType::Mesh)
    {
        // Mesh 模式：设置 task + mesh + fragment shader
        if (mMeshPipelineDes)
        {
            mMeshPipelineDes.objectFunction = shaderPtr->GetTaskFunction();
            mMeshPipelineDes.meshFunction = shaderPtr->GetMeshFunction();
            mMeshPipelineDes.fragmentFunction = shaderPtr->GetFragmentFunction();
        }
    }
    else
    {
        mRenderPipelineDes.vertexFunction = shaderPtr->GetVertexFunction();
        mRenderPipelineDes.fragmentFunction = shaderPtr->GetFragmentFunction();
    }
}

void MTLGraphicsPipeline::AttachTaskShader(ShaderFunctionPtr shaderFunction)
{
    if (!shaderFunction || !mMeshPipelineDes)
    {
        return;
    }
    
    MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(shaderFunction);
    // Metal: Task Shader 对应 Object Shader
    mMeshPipelineDes.objectFunction = shaderPtr->GetShaderFunction();
}

void MTLGraphicsPipeline::AttachMeshShader(ShaderFunctionPtr shaderFunction)
{
    if (!shaderFunction || !mMeshPipelineDes)
    {
        return;
    }
    
    MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(shaderFunction);
    mMeshPipelineDes.meshFunction = shaderPtr->GetShaderFunction();
}

void MTLGraphicsPipeline::Generate(const FrameBufferFormat& frameBufferFormat)
{
    if (mGenerated)
    {
        return;
    }
    
    if (mDesc.pipelineType == PipelineType::Mesh)
    {
        // ============ Mesh Pipeline 分支 ============
        if (mMeshPipelineDes)
        {
            // 设置 framebuffer 格式
            int index = 0;
            for (const auto &iter : frameBufferFormat.colorFormats)
            {
                mMeshPipelineDes.colorAttachments[index++].pixelFormat = iter;
            }
            mMeshPipelineDes.depthAttachmentPixelFormat = frameBufferFormat.depthFormat;
            mMeshPipelineDes.stencilAttachmentPixelFormat = frameBufferFormat.stencilFormat;

            // Note: MTLMeshRenderPipelineDescriptor.binaryArchives is NOT available
            // in macOS 14.0 SDK. Binary Archive hint for Mesh Pipeline will be
            // supported in future SDK versions when Apple adds the property.
            // For now, Mesh Pipeline PSOs are created without cache acceleration.

            NSError *error = nil;
            MTLRenderPipelineReflection* reflectionObj = nil;
            MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
            mMeshPipelineState = [mDevice newRenderPipelineStateWithMeshDescriptor:mMeshPipelineDes options:option
                                                                        reflection:&reflectionObj error:&error];
            
            if (!mMeshPipelineState)
            {
                NSLog(@"创建 Metal Mesh Pipeline 失败: %@", error);
            }
            else
            {
                // 从 PSO 反射属性读取 mesh/task shader 的 threadgroup 大小
                if (@available(macOS 13.0, iOS 16.0, *))
                {
                    // Object (Task) shader: threadsPerObjectThreadgroup
                    uint32_t objExecWidth = (uint32_t)mMeshPipelineState.objectThreadExecutionWidth;
                    uint32_t objMaxTotal  = (uint32_t)mMeshPipelineState.maxTotalThreadsPerObjectThreadgroup;
                    mTaskThreadgroupSize[0] = objExecWidth;
                    mTaskThreadgroupSize[1] = objMaxTotal / objExecWidth;
                    mTaskThreadgroupSize[2] = 1;

                    // Mesh shader: threadsPerMeshThreadgroup
                    uint32_t meshExecWidth = (uint32_t)mMeshPipelineState.meshThreadExecutionWidth;
                    uint32_t meshMaxTotal  = (uint32_t)mMeshPipelineState.maxTotalThreadsPerMeshThreadgroup;
                    mMeshThreadgroupSize[0] = meshExecWidth;
                    mMeshThreadgroupSize[1] = meshMaxTotal / meshExecWidth;
                    mMeshThreadgroupSize[2] = 1;
                }

                // Populate mesh/task resource bindings from reflection
                if (reflectionObj)
                {
#if SUPPORTED_NEW_REFLECT
                    // Extract object (task) shader arguments
                    for (id<MTLBinding> arg in reflectionObj.objectBindings)
                    {
                        NSLog(@"[MeshPipeline] Task arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
                        mTaskBindings[arg.name.UTF8String] = arg.index;
                    }

                    // Extract mesh shader arguments
                    for (id<MTLBinding> arg in reflectionObj.meshBindings)
                    {
                        NSLog(@"[MeshPipeline] Mesh arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
                        mMeshBindings[arg.name.UTF8String] = arg.index;
                    }
#else
                    for (MTLArgument * arg in reflectionObj.objectArguments)
                    {
                        NSLog(@"[MeshPipeline] Task arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
                        mTaskBindings[arg.name.UTF8String] = arg.index;
                    }

                    for (MTLArgument * arg in reflectionObj.meshArguments)
                    {
                        NSLog(@"[MeshPipeline] Mesh arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
                        mMeshBindings[arg.name.UTF8String] = arg.index;
                    }
#endif
                    
                    // 如果通过 AttachGraphicsShader 绑定了 mesh shader，将反射信息也传递给 shader
                    if (mShader && reflectionObj)
                    {
                        mShader->GenerateMeshRefectionInfo(reflectionObj);
                    }
                }

                if (mPipelineCache)
                {
                    // Capture mode：注册到归档
                    mPipelineCache->AddRenderPipelineState(mMeshPipelineState);
                }
            }
        }
    }
    else
    {
        // ============ 传统 Graphics Pipeline 分支 ============
        if (mRenderPipelineDes)
        {
            NSError *error = nil;
            
            //这里再设置renderpass传递过来的frame buffer的格式
            int index = 0;
            for (const auto &iter : frameBufferFormat.colorFormats)
            {
                mRenderPipelineDes.colorAttachments[index++].pixelFormat = iter;
            }
            mRenderPipelineDes.depthAttachmentPixelFormat = frameBufferFormat.depthFormat;
            mRenderPipelineDes.stencilAttachmentPixelFormat = frameBufferFormat.stencilFormat;

            // 附加 Binary Archive hint（Use mode: 加速 PSO 创建）
            // MTLRenderPipelineDescriptor.binaryArchives requires macOS 12.0+
#ifdef SUPPORTED_BINARY_ARCHIVE
            if (mPipelineCache && mPipelineCache->HasValidArchive())
            {
                id<MTLBinaryArchive> archive = mPipelineCache->GetBinaryArchive();
                if (archive)
                {
                    mRenderPipelineDes.binaryArchives = @[archive];
                }
            }
#endif
            
            //创建带反射信息的PSO
            MTLRenderPipelineReflection* reflectionObj = nil;
            MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
            mRenderPipelineState = [mDevice newRenderPipelineStateWithDescriptor:mRenderPipelineDes options:option reflection:&reflectionObj error:&error];
            
            if (mShader)
            {
                mShader->GenerateRefectionInfo(reflectionObj);
            }
            
            uint32_t maxVertexIndex = 0;
            int vertexCount = 0;
            for (MTLArgument *arg in reflectionObj.vertexArguments)
            {
                NSLog(@"Found arg: %@\n", arg.name);
                
                MTLArgumentType argType = arg.type;
                if (MTLArgumentTypeBuffer != argType)
                {
                    continue;
                }
                int index = arg.index;
                MTLArgumentAccess access = arg.access;
                int bufferAlignment = arg.bufferAlignment;
                MTLDataType dataType = arg.bufferDataType;
                int dataSize = arg.bufferDataSize;
                BOOL isActive = arg.isActive;
                
                MTLPointerType * pointerType = arg.bufferPointerType;
                
                if (arg.bufferStructType.members.count == 0)
                {
                    vertexCount++;
                    if (maxVertexIndex < index)
                    {
                        maxVertexIndex = index;
                    }
                }

                for (MTLStructMember* uniform in arg.bufferStructType.members)
                {
                    NSLog(@"uniform: %@ type:%lu, location: %lu", uniform.name, (unsigned long)uniform.dataType, (unsigned long)uniform.offset);
                }
            }
            
            mVertexUniformOffset = maxVertexIndex;
            if (vertexCount > 0)
            {
                mVertexUniformOffset += 1;
            }
            
            if (!mRenderPipelineState)
            {
                NSLog(@"创建metal渲染管线失败: %@", error);
            }
            else if (mPipelineCache)
            {
                // Capture mode：注册到归档
                mPipelineCache->AddRenderPipelineState(mRenderPipelineState);
            }
        }
    }
    
    mGenerated = true;
}

id<MTLRenderPipelineState> MTLGraphicsPipeline::getRenderPipelineState() const
{
    return mRenderPipelineState;
}

MTLGraphicsShaderPtr MTLGraphicsPipeline::GetShader() const
{
    return mShader;
}

NAMESPACE_RENDERCORE_END
