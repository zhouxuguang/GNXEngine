//
//  MTLShaderFunction.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_SHADER_FUNCTION_INCLUDE_H
#define GNX_ENGINE_SHADER_FUNCTION_INCLUDE_H

#include "MTLRenderDefine.h"
#include "ShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

// 需要注意 : std::enable_shared_from_this 要public继承
class MTLShaderFunction : public ShaderFunction , public std::enable_shared_from_this<MTLShaderFunction>
{
public:
    MTLShaderFunction(id<MTLDevice> device);
    
    ~MTLShaderFunction();
    
    virtual ShaderFunctionPtr initWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual ShaderStage getShaderStage() const;
    
    id<MTLFunction> getShaderFunction() const;
    
private:
    ShaderStage mShaderStage;
    id<MTLLibrary> mLibrary;
    id<MTLDevice> mDevice;
    id<MTLFunction> mFunction;
};

typedef std::shared_ptr<MTLShaderFunction> MTLShaderFunctionPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_SHADER_FUNCTION_INCLUDE_H */
