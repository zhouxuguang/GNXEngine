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
struct StencilDescriptor
{
    bool stencilEnable = false;
    CompareFunction stencilCompareFunction = CompareFunctionAlways;
    StencilOperation stencilFailureOperation = StencilOperationKeep;       //模板测试失败的操作
    StencilOperation depthFailureOperation = StencilOperationKeep;         //模板测试通过但是没有通过深度测试的操作
    StencilOperation depthStencilPassOperation = StencilOperationKeep;     //模板测试和深度测试都通过时的操作
    uint32_t readMask = 255;
    uint32_t writeMask = 255;
public:
    bool operator == (const StencilDescriptor& des) const
    {
        return stencilEnable == des.stencilEnable && stencilCompareFunction == des.stencilCompareFunction &&
        stencilFailureOperation == des.stencilFailureOperation &&
        depthFailureOperation == des.depthFailureOperation &&
        depthStencilPassOperation == des.depthStencilPassOperation &&
        readMask == des.readMask && writeMask == des.writeMask;
    }
    
    StencilDescriptor& operator=(const StencilDescriptor& des)
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
struct DepthStencilDescriptor
{
    CompareFunction depthCompareFunction = CompareFunctionAlways;
    bool depthWriteEnabled = true;
    StencilDescriptor stencil;
public:
    bool operator == (const DepthStencilDescriptor& des) const
    {
        return depthCompareFunction == des.depthCompareFunction &&
        depthWriteEnabled == des.depthWriteEnabled &&
        stencil == des.stencil;
    }
    
    DepthStencilDescriptor& operator=(const DepthStencilDescriptor& des)
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
struct VertextAttributesDescritptor
{
    uint32_t index = 0;                                        //buffer的索引
    VertexFormat format = VertexFormatInvalid;                    //属性的格式
    uint32_t offset = 0;                                       //顶点属性在属性的偏移
    //char* attrName = nullptr;                                       //属性的名字
public:
    bool operator == (const VertextAttributesDescritptor& des) const
    {
        return index == des.index && format == des.format && offset == des.offset /*&& strcmp(attrName, des.attrName) == 0*/;
    }
};

/**
 描述顶点的跨距信息
 */
struct VertexBufferLayoutDescriptor
{
    uint32_t stride = 0;
    uint32_t stepRate;
    VertextStepFunc stepFunction;
public:
    bool operator == (const VertexBufferLayoutDescriptor& des) const
    {
        return stride == des.stride && stepRate == des.stepRate &&
        stepFunction == des.stepFunction;
    }
};

/**
 顶点的描述信息
 */
struct VertexDescriptor
{
    std::vector<VertextAttributesDescritptor> attributes;
    std::vector<VertexBufferLayoutDescriptor> layouts;
public:
    bool operator == (const VertexDescriptor& des) const
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
};

/**
 纹理采样器的描述
 */
struct SamplerDescriptor
{
    SamplerMagFilter filterMag = MAG_LINEAR;    // NEAREST
    SamplerMinFilter filterMin = MIN_LINEAR;    // NEAREST
    SamplerWrapMode wrapS = CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    SamplerWrapMode wrapT = CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    SamplerWrapMode wrapR = CLAMP_TO_EDGE;    // CLAMP_TO_EDGE
    uint8_t anisotropyLog2  = 0;
    SamplerCompareMode compareMode = NONE;    // NONE
    CompareFunction compareFunc = CompareFunctionLessThanOrEqual;    // LE
    uint8_t minLod = 0;
    uint8_t maxLod = 0;
public:
    SamplerDescriptor(){}
    SamplerDescriptor(SamplerMagFilter magfiler,SamplerMinFilter minfilter,SamplerWrapMode wrapS,
            SamplerWrapMode wrapT, uint8_t minLod, uint8_t maxLod)
    {
        this->filterMag = magfiler;
        this->filterMin = minfilter;
        this->wrapS = wrapS;
        this->wrapT = wrapT;
        this->minLod = minLod;
        this->maxLod = maxLod;
    }

    SamplerDescriptor(SamplerMagFilter magfiler,SamplerMinFilter minfilter,SamplerWrapMode wrapS,SamplerWrapMode wrapT)
    {
        this->filterMag = magfiler;
        this->filterMin = minfilter;
        this->wrapS = wrapS;
        this->wrapT = wrapT;
    }
    
    SamplerDescriptor(SamplerWrapMode wrapS, SamplerWrapMode wrapT)
    {
        this->wrapS = wrapS;
        this->wrapT = wrapT;
    }
    
    SamplerDescriptor(SamplerMagFilter magfiler, SamplerMinFilter minfilter)
    {
        this->filterMag = magfiler;
        this->filterMin = minfilter;
    }
    
    bool operator == (const SamplerDescriptor& des) const
    {
        return filterMin==des.filterMin &&
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
struct TextureDescriptor
{
    TextureFormat format = kTexFormatRGBA32;
    TextureUsage usage = TextureUsageShaderRead;
    bool mipmaped = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t bytesPerRow = 0;
//public:
//    int getBytesPerPixel() const
//    {
//        int bytes = 0;
//        switch (format)
//        {
//            case kTexFormatAlpha8:
//            case kTexFormatLuma:
//                bytes = 1;
//                break;
//            case kTexFormatRGBA4444:
//            case kTexFormatRGBA5551:
//            case kTexFormatRGB565:
//            case kTexFormatAlphaLum16:
//                bytes = 2;
//                break;
//            case kTexFormatRGBA32:
//                bytes = 4;
//                break;
//            default:
//                break;
//        }
//        return bytes;
//    }
};

struct ColorAttachmentDescriptor
{
    bool blendingEnabled = false;
    BlendFactor sourceRGBBlendFactor = BlendFactorSourceAlpha;
    BlendFactor destinationRGBBlendFactor = BlendFactorOneMinusSourceAlpha;
    BlendEquation rgbBlendOperation = BlendEquationAdd;
    BlendFactor sourceAlphaBlendFactor = BlendFactorSourceAlpha;
    BlendFactor destinationAplhaBlendFactor = BlendFactorOneMinusSourceAlpha;
    BlendEquation aplhaBlendOperation = BlendEquationAdd;
    ColorWriteMask writeMask = ColorWriteMaskAll;
    
    bool operator==(const ColorAttachmentDescriptor& des) const
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
    
    static ColorAttachmentDescriptor getDisableDes()
    {
        return ColorAttachmentDescriptor();
    }
    
    static ColorAttachmentDescriptor getCommonBlendDes()
    {
        ColorAttachmentDescriptor colorDes;
        colorDes.blendingEnabled = true;
        return colorDes;
    }
    
    static ColorAttachmentDescriptor getPreMultilyAlphaBlendDes()
    {
        ColorAttachmentDescriptor colorDes;
        colorDes.blendingEnabled = true;
        colorDes.sourceRGBBlendFactor = BlendFactorOne;
        return colorDes;
    }
};

/**
 渲染管线的描述
 */
struct GraphicsPipelineDescriptor
{
    VertexDescriptor vertexDescriptor;                      //顶点buffer数据描述
    ColorAttachmentDescriptor colorAttachmentDescriptor;        //颜色相关描述
    DepthStencilDescriptor depthStencilDescriptor;             //深度模板测试状态
public:
    bool operator == (const GraphicsPipelineDescriptor& des) const
    {
        return vertexDescriptor == des.vertexDescriptor &&
        colorAttachmentDescriptor == des.colorAttachmentDescriptor &&
        depthStencilDescriptor == des.depthStencilDescriptor;
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
