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
    VKShaderFunction(VulkanContextPtr context);
    ~VKShaderFunction();
    
    virtual ShaderFunctionPtr initWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual std::shared_ptr<VKShaderFunction> initWithShaderSourceInner(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual ShaderStage getShaderStage() const;
    
    VkShaderModule GetShaderModule() const
    {
        return mShaderFunction;
    }
    
private:
    VulkanContextPtr mContext = nullptr;
    VkShaderModule mShaderFunction = VK_NULL_HANDLE;
    ShaderStage mShaderStage;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_SHADER_FUNCTION_INCLUDE_SDFJH */
