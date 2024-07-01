//
//  MTLGraphicsPipeline.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/28.
//

#include "MTLGraphicsPipeline.h"
#include "MTLShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

static id<MTLDepthStencilState> convertToMTLDSDes(id<MTLDevice> device, const DepthStencilDescriptor& depthStencilDescriptor)
{
    MTLDepthStencilDescriptor* mtlDescriptor = [MTLDepthStencilDescriptor new];
    MTLCompareFunction compareFunc = (MTLCompareFunction)depthStencilDescriptor.depthCompareFunction;
    //只有不为always次设置对应字段
    if (compareFunc != MTLCompareFunctionAlways)
    {
        mtlDescriptor.depthCompareFunction = compareFunc;
        mtlDescriptor.depthWriteEnabled = depthStencilDescriptor.depthWriteEnabled;
    }
    
    
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
            
        default:
            break;
    }
    
    return mtlFormat;
}

static MTLRenderPipelineDescriptor* convertToMTLRenderPiplineDescriptor(const GraphicsPipelineDescriptor& des)
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
    
    //mtlPiplineDes.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
    mtlPiplineDes.colorAttachments[0].blendingEnabled = des.colorAttachmentDescriptor.blendingEnabled;
    mtlPiplineDes.colorAttachments[0].sourceRGBBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptor.sourceRGBBlendFactor;
    mtlPiplineDes.colorAttachments[0].destinationRGBBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptor.destinationRGBBlendFactor;
    mtlPiplineDes.colorAttachments[0].rgbBlendOperation = (MTLBlendOperation)des.colorAttachmentDescriptor.rgbBlendOperation;
    mtlPiplineDes.colorAttachments[0].sourceAlphaBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptor.sourceAlphaBlendFactor;
    mtlPiplineDes.colorAttachments[0].destinationAlphaBlendFactor = (MTLBlendFactor)des.colorAttachmentDescriptor.destinationAplhaBlendFactor;
    mtlPiplineDes.colorAttachments[0].alphaBlendOperation = (MTLBlendOperation)des.colorAttachmentDescriptor.aplhaBlendOperation;
    mtlPiplineDes.colorAttachments[0].writeMask = (MTLColorWriteMask)des.colorAttachmentDescriptor.writeMask;
    
    //这里格式还需要再考察，得和实际使用的一样
//    mtlPiplineDes.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
//    mtlPiplineDes.stencilAttachmentPixelFormat = MTLPixelFormatStencil8;
    
    return mtlPiplineDes;
}

MTLGraphicsPipeline::MTLGraphicsPipeline(id<MTLDevice> device, const GraphicsPipelineDescriptor& des) : GraphicsPipeline(des)
{
    mDevice = device;
    mRenderPipelineDes = convertToMTLRenderPiplineDescriptor(des);
    mDepthStencilState = convertToMTLDSDes(device, des.depthStencilDescriptor);
}

MTLGraphicsPipeline::~MTLGraphicsPipeline()
{
    //
}

void MTLGraphicsPipeline::attachVertexShader(ShaderFunctionPtr shaderFunction)
{
    if (!shaderFunction)
    {
        return;
    }
    
    MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(shaderFunction);
    mRenderPipelineDes.vertexFunction = shaderPtr->getShaderFunction();
}

void MTLGraphicsPipeline::attachFragmentShader(ShaderFunctionPtr shaderFunction)
{
    if (!shaderFunction)
    {
        return;
    }
    
    MTLShaderFunctionPtr shaderPtr = std::dynamic_pointer_cast<MTLShaderFunction>(shaderFunction);
    mRenderPipelineDes.fragmentFunction = shaderPtr->getShaderFunction();
}

static void GetShaderReflectionInfo(MTLRenderPipelineReflection* reflectionObj)
{
    for (MTLArgument *arg in reflectionObj.vertexArguments)
    {
        NSLog(@"Found arg: %@\n", arg.name);
        
        MTLArgumentType argType = arg.type;
        int index = arg.index;
        MTLArgumentAccess access = arg.access;
        int bufferAlignment = arg.bufferAlignment;
        MTLDataType dataType = arg.bufferDataType;
        int dataSize = arg.bufferDataSize;
        BOOL isActive = arg.isActive;
        
        MTLPointerType * pointerType = arg.bufferPointerType;
        MTLDataType dataType1 = pointerType.dataType;
        MTLDataType dataType2 = pointerType.elementType;
        BOOL isConstBuffer = pointerType.elementIsArgumentBuffer;
        
        //NSArray<NSString *> *assar = arg.bufferStructType.attributeKeys;
        
        MTLDataType dataType3 = pointerType.elementStructType.dataType;
        MTLDataType dataType4 = arg.bufferPointerType.elementStructType.dataType;
        
//        for (MTLStructMember* uniform in )
//        {
//            MTLDataType dataType = pointerType.elementArrayType.dataType;
//            uint32_t stride = pointerType.elementArrayType.stride;
//            uint32_t arrayLength = pointerType.elementArrayType.arrayLength;
//            NSString *name = @"";
//            //NSLog(@"vertex buffer: %@ type:%lu, location: %lu", uniform.name, (unsigned long)uniform.dataType, (unsigned long)uniform.offset);
//        }
        
        
        
        if (arg.bufferStructType.members.count == 0)
        {
            //count++;
        }

        for (MTLStructMember* uniform in arg.bufferStructType.members)
        {
            NSLog(@"uniform: %@ type:%lu, location: %lu", uniform.name, (unsigned long)uniform.dataType, (unsigned long)uniform.offset);
        }
    }
    
#if 0
    for (id<MTLBinding> arg in reflectionObj.vertexBindings)
    {
        NSLog(@"Found arg: %@\n", arg.name);
        
        MTLBindingType argType = arg.type;
        int index = arg.index;
        MTLArgumentAccess access = arg.access;
        BOOL used = arg.isUsed;
        BOOL arugment = arg.isArgument;   //arugment为true则是uniformbuffer
        NSString* descStr = arg.debugDescription;
        NSString* descStr1 = arg.description;
        
//        for (MTLStructMember* uniform in arg..members)
//        {
//            NSLog(@"uniform: %@ type:%lu, location: %lu", uniform.name, (unsigned long)uniform.dataType, (unsigned long)uniform.offset);
//        }
    }
#endif
}

void MTLGraphicsPipeline::Generate(const FrameBufferFormat& frameBufferFormat)
{
    if (mGenerated)
    {
        return;
    }
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
        
        //创建带反射信息的PSO
        MTLRenderPipelineReflection* reflectionObj = nil;
        MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
        mRenderPipelineState = [mDevice newRenderPipelineStateWithDescriptor:mRenderPipelineDes options:option reflection:&reflectionObj error:&error];
        mReflectionObj = reflectionObj;
        
        GetShaderReflectionInfo(reflectionObj);
        
        //
        uint32_t maxVertexIndex = 0;
        int vertexCount = 0;
        for (MTLArgument *arg in reflectionObj.vertexArguments)
        {
            NSLog(@"Found arg: %@\n", arg.name);
            
            MTLArgumentType argType = arg.type;
            int index = arg.index;
            MTLArgumentAccess access = arg.access;
            int bufferAlignment = arg.bufferAlignment;
            MTLDataType dataType = arg.bufferDataType;
            int dataSize = arg.bufferDataSize;
            BOOL isActive = arg.isActive;
            
            MTLPointerType * pointerType = arg.bufferPointerType;
            
//            MTLArgumentType type = arg.type;
//            MTLArgumentAccess access = arg.access;
            
            if (arg.bufferStructType.members.count == 0)
            {
                vertexCount ++;
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
        
#if 0
        //新版的反射信息
        for (id<MTLBinding> arg in reflectionObj.vertexBindings)
        {
            NSLog(@"Found arg: %@\n", arg.name);
            
            MTLBindingType argType = arg.type;
            int index = arg.index;
            MTLArgumentAccess access = arg.access;
            BOOL used = arg.isUsed;
            BOOL arugment = arg.isArgument;   //arugment为true则是uniformbuffer
            NSString* descStr = arg.debugDescription;
            NSString* descStr1 = arg.description;
            
//            for (MTLStructMember* uniform in arg.bufferStructType.members)
//            {
//                NSLog(@"uniform: %@ type:%lu, location: %lu", uniform.name, (unsigned long)uniform.dataType, (unsigned long)uniform.offset);
//            }
            
            id<MTLBufferBinding> bufferBinding = arg;
            MTLDataType dataType = bufferBinding.bufferDataType;
            uint32_t dataSize = bufferBinding.bufferDataSize;
            
            int a = 10;
        }
        
#endif
        
        mVertexUniformOffset = maxVertexIndex;
        if (vertexCount > 0)
        {
            mVertexUniformOffset += 1;
        }
        
        if (!mRenderPipelineState)
        {
            NSLog(@"创建metal渲染管线失败: %@", error);
        }
        mGenerated = true;
    }
}

id<MTLRenderPipelineState> MTLGraphicsPipeline::getRenderPipelineState() const
{
    return mRenderPipelineState;
}

NAMESPACE_RENDERCORE_END
