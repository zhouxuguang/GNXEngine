//
//  VKShaderFunction.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

VKShaderFunction::VKShaderFunction(VulkanContextPtr context)
{
    mContext = context;
}

VKShaderFunction::~VKShaderFunction()
{
    if (mShaderFunction != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(mContext->device, mShaderFunction, nullptr);
    }
}

std::shared_ptr<ShaderFunction> VKShaderFunction::initWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderSource.size();
    shaderModuleCreateInfo.pCode = (const uint32_t*)shaderSource.data();
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(mContext->device, &shaderModuleCreateInfo, nullptr, &computeShaderModule);
    if (res != VK_SUCCESS)
    {
        mShaderFunction = VK_NULL_HANDLE;
        return nullptr;
    }
    mShaderFunction = computeShaderModule;
    
    return shared_from_this();
}

std::shared_ptr<VKShaderFunction> VKShaderFunction::initWithShaderSourceInner(const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderSource.size();
    shaderModuleCreateInfo.pCode = (const uint32_t*)shaderSource.data();
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(mContext->device, &shaderModuleCreateInfo, nullptr, &computeShaderModule);
    if (res != VK_SUCCESS)
    {
        mShaderFunction = VK_NULL_HANDLE;
        return nullptr;
    }
    mShaderFunction = computeShaderModule;
    
    return shared_from_this();
}

ShaderStage VKShaderFunction::getShaderStage() const
{
    return mShaderStage;
}

NAMESPACE_RENDERCORE_END
