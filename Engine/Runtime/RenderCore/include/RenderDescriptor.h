//
//  RenderDescriptor.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/7/5.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#ifndef GNX_ENGINE_RENDER_DESCRIPTOR_INCLUDE_H
#define GNX_ENGINE_RENDER_DESCRIPTOR_INCLUDE_H

#include "RenderDefine.h"
#include <vector>
#include "TextureFormat.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 模板测试描述信息
 */
struct StencilDesc
{
    bool stencilEnable = false;
    CompareFunction stencilCompareFunction = CompareFunctionNever;
    StencilOperation stencilFailureOperation = StencilOperationKeep;       //模板测试失败的操作
    StencilOperation depthFailureOperation = StencilOperationKeep;         //模板测试通过但是没有通过深度测试的操作
    StencilOperation depthStencilPassOperation = StencilOperationKeep;     //模板测试和深度测试都通过时的操作
    uint32_t readMask = 255;
    uint32_t writeMask = 255;
public:
    bool operator == (const StencilDesc& des) const
    {
        return stencilEnable == des.stencilEnable && stencilCompareFunction == des.stencilCompareFunction &&
            stencilFailureOperation == des.stencilFailureOperation &&
            depthFailureOperation == des.depthFailureOperation &&
            depthStencilPassOperation == des.depthStencilPassOperation &&
            readMask == des.readMask && writeMask == des.writeMask;
    }
    
    StencilDesc& operator = (const StencilDesc& des)
    {
        stencilEnable = des.stencilEnable;
        stencilCompareFunction = des.stencilCompareFunction;
        stencilFailureOperation = des.stencilFailureOperation;
        depthFailureOperation = des.depthFailureOperation;
        depthStencilPassOperation = des.depthStencilPassOperation;
        readMask = des.readMask;
        writeMask = des.writeMask;
        return *this;
    }
};

/**
 深度和模板测试描述信息
 */
struct DepthStencilDesc
{
    CompareFunction depthCompareFunction = CompareFunctionAlways;
    bool depthWriteEnabled = true;
    StencilDesc stencil;
public:
    bool operator == (const DepthStencilDesc& des) const
    {
        return depthCompareFunction == des.depthCompareFunction &&
        depthWriteEnabled == des.depthWriteEnabled &&
        stencil == des.stencil;
    }
    
    bool operator != (const DepthStencilDesc& des) const
    {
        return !(*this == des);
    }
    
    DepthStencilDesc& operator=(const DepthStencilDesc& des)
    {
        depthCompareFunction = des.depthCompareFunction;
        depthWriteEnabled = des.depthWriteEnabled;
        stencil = des.stencil;
        
        return *this;
    }
};

/**
 描述顶点属性的索引、格式等信息
 */
struct VertextAttributesDesc
{
    uint32_t index = 0;                                        //buffer的索引
    VertexFormat format = VertexFormatInvalid;                    //属性的格式
    uint32_t offset = 0;                                       //顶点属性在属性的偏移
    //char* attrName = nullptr;                                       //属性的名字
public:
    bool operator == (const VertextAttributesDesc& des) const
    {
        return index == des.index && format == des.format && offset == des.offset /*&& strcmp(attrName, des.attrName) == 0*/;
    }
};

/**
 描述顶点的跨距信息
 */
struct VertexBufferLayoutDesc
{
    uint32_t stride = 0;
    uint32_t stepRate;
    VertextStepFunc stepFunction;
public:
    bool operator == (const VertexBufferLayoutDesc& des) const
    {
        return stride == des.stride && stepRate == des.stepRate &&
            stepFunction == des.stepFunction;
    }
};

/**
 顶点的描述信息
 */
struct VertexDesc
{
    std::vector<VertextAttributesDesc> attributes;
    std::vector<VertexBufferLayoutDesc> layouts;
public:
    bool operator == (const VertexDesc& des) const
    {
        if (attributes.size() != des.attributes.size())
        {
            return false;
        }
        
        for (const auto& iter1 : attributes)
        {
            bool find = false;
            for (const auto& iter2 : des.attributes)
            {
                if (iter1 == iter2)
                {
                    find = true;
                    break;
                }
            }
            if (!find)
                return false;
        }
        
        for (const auto& iter1 : layouts)
        {
            bool find = false;
            for (const auto& iter2 : des.layouts)
            {
                if (iter1 == iter2)
                {
                    find = true;
                    break;
                }
            }
            if (!find)
                return false;
        }
        return true;
    }
    
    bool operator != (const VertexDesc& des) const
    {
        return !(*this == des);
    }
};

/**
 纹理采样器的描述
 */
struct SamplerDesc
{
    SamplerMagFilter filterMag = MAG_LINEAR;    // NEAREST
    SamplerMinFilter filterMin = MIN_LINEAR;    // NEAREST
    SamplerMipFilter filterMip = MIN_LINEAR_MIPMAP_LINEAR;
    SamplerWrapMode wrapS = CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    SamplerWrapMode wrapT = CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    SamplerWrapMode wrapR = CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    uint8_t anisotropyLog2  = 0;
    SamplerCompareMode compareMode = NONE;    // NONE
    CompareFunction compareFunc = CompareFunctionLessThanOrEqual;    // LE
    uint8_t minLod = 0;
    uint8_t maxLod = 0;
public:
    SamplerDesc(){}
    SamplerDesc(SamplerMagFilter magfiler, SamplerMinFilter minfilter, SamplerWrapMode wrapS,
            SamplerWrapMode wrapT, uint8_t minLod, uint8_t maxLod)
    {
        this->filterMag = magfiler;
        this->filterMin = minfilter;
        this->wrapS = wrapS;
        this->wrapT = wrapT;
        this->minLod = minLod;
        this->maxLod = maxLod;
    }

    SamplerDesc(SamplerMagFilter magfiler, SamplerMinFilter minfilter, SamplerWrapMode wrapS, SamplerWrapMode wrapT)
    {
        this->filterMag = magfiler;
        this->filterMin = minfilter;
        this->wrapS = wrapS;
        this->wrapT = wrapT;
    }
    
    SamplerDesc(SamplerWrapMode wrapS, SamplerWrapMode wrapT)
    {
        this->wrapS = wrapS;
        this->wrapT = wrapT;
    }
    
    SamplerDesc(SamplerMagFilter magfiler, SamplerMinFilter minfilter)
    {
        this->filterMag = magfiler;
        this->filterMin = minfilter;
    }
    
    bool operator == (const SamplerDesc& des) const
    {
        return filterMin == des.filterMin &&
        filterMag == des.filterMag &&
        wrapS == des.wrapS &&
        wrapR == des.wrapR &&
        wrapT == des.wrapT &&
        anisotropyLog2 == des.anisotropyLog2 &&
        compareMode == des.compareMode &&
        compareFunc == des.compareFunc &&
        minLod == des.minLod &&
        maxLod == des.maxLod;
    }
};

/**
 纹理描述信息
 */
struct TextureDesc
{
    TextureFormat format = kTexFormatRGBA8;
    TextureUsage usage = TextureUsage::TextureUsageShaderRead;
    bool mipmaped = false;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t bytesPerRow = 0;
};

struct ColorAttachmentDesc
{
    bool blendingEnabled = false;
    BlendFactor sourceRGBBlendFactor = BlendFactorSourceAlpha;
    BlendFactor destinationRGBBlendFactor = BlendFactorOneMinusSourceAlpha;
    BlendEquation rgbBlendOperation = BlendEquationAdd;
    BlendFactor sourceAlphaBlendFactor = BlendFactorSourceAlpha;
    BlendFactor destinationAplhaBlendFactor = BlendFactorOneMinusSourceAlpha;
    BlendEquation aplhaBlendOperation = BlendEquationAdd;
    ColorWriteMask writeMask = ColorWriteMaskAll;
    
    bool operator == (const ColorAttachmentDesc& des) const
    {
        return blendingEnabled == des.blendingEnabled &&
        sourceRGBBlendFactor == des.sourceRGBBlendFactor &&
        destinationRGBBlendFactor == des.destinationRGBBlendFactor &&
        rgbBlendOperation == des.rgbBlendOperation &&
        sourceAlphaBlendFactor == des.sourceAlphaBlendFactor &&
        destinationAplhaBlendFactor == des.destinationAplhaBlendFactor &&
        aplhaBlendOperation == des.aplhaBlendOperation &&
        writeMask == des.writeMask;
    }
    
    bool operator != (const ColorAttachmentDesc& des) const
    {
        return !(*this == des);
    }
    
    static ColorAttachmentDesc GetDisableDes()
    {
        return ColorAttachmentDesc();
    }
    
    static ColorAttachmentDesc GetCommonBlendDes()
    {
        ColorAttachmentDesc colorDes;
        colorDes.blendingEnabled = true;
        return colorDes;
    }
    
    static ColorAttachmentDesc GetPreMultilyAlphaBlendDes()
    {
        ColorAttachmentDesc colorDes;
        colorDes.blendingEnabled = true;
        colorDes.sourceRGBBlendFactor = BlendFactorOne;
        return colorDes;
    }
};

static const uint32_t MAX_COLOR_ATTACHMENT_COUNT = 16;

/**
 * Mesh shader 间接绘制命令
 */
struct DrawMeshTasksIndirectCommand
{
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

/**
 渲染管线的描述
 */
struct GraphicsPipelineDesc
{
    PipelineType pipelineType = PipelineType::Graphics;

    // 以下字段在 PipelineType::Graphics 时使用
    VertexDesc vertexDescriptor;                                                  //顶点buffer数据描述

    // 以下字段在 PipelineType::Mesh 时使用
    uint32_t meshThreadgroupSizeX = 128;    // 每个mesh threadgroup的线程数
    uint32_t meshThreadgroupSizeY = 1;
    uint32_t meshThreadgroupSizeZ = 1;
    uint32_t taskPayloadSize = 0;           // Vulkan Task Shader payload 大小（Metal 忽略）
    uint32_t maxObjectPayloadMeshlets = 0;  // Metal maxTotalThreadgroupsPerMeshGrid（Vulkan 忽略）

    // 以下两种模式共用
    uint32_t renderTargetCount = 1;
    ColorAttachmentDesc colorAttachmentDescriptors[MAX_COLOR_ATTACHMENT_COUNT];   //颜色相关描述
    DepthStencilDesc depthStencilDescriptor;                                      //深度模板测试状态
    FillMode fillMode = FillModeSolid;                                            //多边形填充模式
public:
    bool operator == (const GraphicsPipelineDesc& des) const
    {
        if (pipelineType != des.pipelineType)
        {
            return false;
        }
        if (pipelineType == PipelineType::Graphics && vertexDescriptor != des.vertexDescriptor)
        {
            return false;
        }
        if (pipelineType == PipelineType::Mesh)
        {
            if (meshThreadgroupSizeX != des.meshThreadgroupSizeX ||
                meshThreadgroupSizeY != des.meshThreadgroupSizeY ||
                meshThreadgroupSizeZ != des.meshThreadgroupSizeZ ||
                taskPayloadSize != des.taskPayloadSize ||
                maxObjectPayloadMeshlets != des.maxObjectPayloadMeshlets)
            {
                return false;
            }
        }
        if (depthStencilDescriptor != des.depthStencilDescriptor)
        {
            return false;
        }
        if (fillMode != des.fillMode)
        {
            return false;
        }
        if (renderTargetCount != des.renderTargetCount)
        {
            return false;
        }
        
        for (uint32_t i = 0; i < renderTargetCount; i ++)
        {
            if (colorAttachmentDescriptors[i] != des.colorAttachmentDescriptors[i])
            {
                return false;
            }
        }
        
        return true;
    }
};

/**
 深度偏移描述
 */
struct DepthBias
{
    DepthBias()
    {
        
    }
    DepthBias(float fa, float un) : factor(fa),unite(un)
    {
        
    }
    float factor = 0.0f;
    float unite = 0.0f;

public:
    bool operator == (const DepthBias& rhs) const
    {
        return true;
    }
    
    bool operator != (const DepthBias& rhs) const
    {
        return !((*this)==rhs);
    }
    
    bool isDepthBiasNeeded() const
    {
        return true;
    }
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RENDER_DESCRIPTOR_INCLUDE_H */
