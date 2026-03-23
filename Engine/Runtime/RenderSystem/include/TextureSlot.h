//
//  TextureSlot.h
//  GNXEngine
//
//  纹理槽 - 纹理与采样器描述的配对
//

#ifndef GNXENGINE_TEXTURE_SLOT_INCLUDE_H
#define GNXENGINE_TEXTURE_SLOT_INCLUDE_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/RenderDescriptor.h"

NS_RENDERSYSTEM_BEGIN

/**
 * 纹理槽 - 封装纹理与采样器描述的配对
 * 
 * 用于材质系统中存储纹理及其采样参数，
 * 渲染Pass根据SamplerDesc创建/复用采样器对象。
 */
struct TextureSlot
{
    RenderCore::RCTexturePtr texture;
    RenderCore::SamplerDesc samplerDesc;
    
    TextureSlot() = default;
    
    TextureSlot(RenderCore::RCTexturePtr tex, const RenderCore::SamplerDesc& desc)
        : texture(tex), samplerDesc(desc) {}
    
    explicit TextureSlot(RenderCore::RCTexturePtr tex)
        : texture(tex), samplerDesc() {}
    
    bool IsValid() const { return texture != nullptr; }
    
    // 默认采样器描述（线性过滤，clamp to edge）
    static RenderCore::SamplerDesc GetDefaultSamplerDesc()
    {
        return RenderCore::SamplerDesc();
    }
    
    // 创建常用采样器描述
    static RenderCore::SamplerDesc CreateLinearClampSampler()
    {
        return RenderCore::SamplerDesc();
    }
    
    static RenderCore::SamplerDesc CreateLinearRepeatSampler()
    {
        return RenderCore::SamplerDesc(
            RenderCore::MAG_LINEAR,
            RenderCore::MIN_LINEAR,
            RenderCore::REPEAT,
            RenderCore::REPEAT
        );
    }
    
    static RenderCore::SamplerDesc CreateNearestClampSampler()
    {
        return RenderCore::SamplerDesc(
            RenderCore::MAG_NEAREST,
            RenderCore::MIN_NEAREST
        );
    }
    
    static RenderCore::SamplerDesc CreateShadowSampler()
    {
        RenderCore::SamplerDesc desc;
        desc.compareMode = RenderCore::COMPARE_TO_TEXTURE;
        desc.compareFunc = RenderCore::CompareFunctionLess;
        return desc;
    }
};

NS_RENDERSYSTEM_END

#endif // GNXENGINE_TEXTURE_SLOT_INCLUDE_H
