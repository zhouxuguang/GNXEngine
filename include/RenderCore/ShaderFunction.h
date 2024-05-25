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

class ShaderFunction;
typedef std::shared_ptr<ShaderFunction> ShaderFunctionPtr;

class ShaderFunction
{
public:
    ShaderFunction();
    
    virtual ~ShaderFunction();
    
    virtual ShaderFunctionPtr initWithShaderSource(const char* pShaderSource, ShaderStage shaderStage) = 0;
    
    virtual ShaderStage getShaderStage() const = 0;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_SHADE_FUNCTION_INCLUDE_H */
