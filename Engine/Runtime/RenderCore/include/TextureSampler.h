//
//  TextureSampler.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/4/29.
//

#ifndef GNX_ENGINE_TEXTURE_INCLUDE_DJGJDFJJ_H
#define GNX_ENGINE_TEXTURE_INCLUDE_DJGJDFJJ_H

#include "RenderDescriptor.h"

NAMESPACE_RENDERCORE_BEGIN

class TextureSampler
{
public:
    
    /**
     *  创建默认 sampler.
     * The default parameters are:
     * - filterMag      : NEAREST
     * - filterMin      : NEAREST
     * - wrapS          : CLAMP_TO_EDGE
     * - wrapT          : CLAMP_TO_EDGE
     * - wrapR          : CLAMP_TO_EDGE
     * - compareMode    : NONE
     * - compareFunc    : Less or equal
     * - no anisotropic filtering
     */
    TextureSampler(const SamplerDescriptor& des);
    
    virtual ~TextureSampler();
};

typedef std::shared_ptr<TextureSampler> TextureSamplerPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_TEXTURE_INCLUDE_DJGJDFJJ_H */
