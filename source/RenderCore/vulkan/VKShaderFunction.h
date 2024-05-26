//
//  VKShaderFunction.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH
#define GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH

#include "VulkanContext.h"
#include "ShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

class VKShaderFunction : public ShaderFunction, public std::enable_shared_from_this<VKShaderFunction>
{
public:
    VKShaderFunction();
    ~VKShaderFunction();
    
    virtual std::shared_ptr<ShaderFunction> initWithShaderSource(const char* pShaderSource, ShaderStage shaderStage) = 0;
    
    virtual ShaderStage getShaderStage() const = 0;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH */
