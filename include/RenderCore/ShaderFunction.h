//
//  ShaderFunction.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/3.
//

#ifndef GNX_ENGINE_SHADE_FUNCTION_INCLUDE_H
#define GNX_ENGINE_SHADE_FUNCTION_INCLUDE_H

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

using ShaderCode = std::vector<uint8_t>;

// 单个的shader函数对象，例如vs/ps等
class ShaderFunction
{
public:
    ShaderFunction();
    
    virtual ~ShaderFunction();
    
    virtual std::shared_ptr<ShaderFunction> initWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage) = 0;
    
    virtual ShaderStage getShaderStage() const = 0;
};

using ShaderFunctionPtr = std::shared_ptr<ShaderFunction>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_SHADE_FUNCTION_INCLUDE_H */
