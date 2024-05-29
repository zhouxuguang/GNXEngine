//
//  VKShaderFunction.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#include "VKShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

VKShaderFunction::VKShaderFunction(VulkanContextPtr context, const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    //
}

VKShaderFunction::~VKShaderFunction()
{
    //
}

std::shared_ptr<ShaderFunction> VKShaderFunction::initWithShaderSource(const char* pShaderSource, ShaderStage shaderStage)
{
    return nullptr;
}

ShaderStage VKShaderFunction::getShaderStage() const
{
    return ShaderStage_Vertex;
}

NAMESPACE_RENDERCORE_END
