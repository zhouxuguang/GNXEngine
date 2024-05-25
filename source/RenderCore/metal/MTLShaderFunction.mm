//
//  MTLShaderFunction.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLShaderFunction.h"

NAMESPACE_RENDERCORE_BEGIN

MTLShaderFunction::MTLShaderFunction(id<MTLDevice> device)
{
    mDevice = device;
    //mLibrary = library;
}

MTLShaderFunction::~MTLShaderFunction()
{
    //
}

static const char* getFunctionName(ShaderStage shaderStage)
{
    switch (shaderStage)
    {
        case ShaderStage_Vertex:
            return "VS";
            
        case ShaderStage_Fragment:
            return "PS";
            
        case ShaderStage_Compute:
            return "CS";
            
        default:
            break;
    }
    
    return "";
}

ShaderFunctionPtr MTLShaderFunction::initWithShaderSource(const char* pShaderSource, ShaderStage shaderStage)
{
    if (!pShaderSource)
    {
        return nullptr;
    }
    
    NSError *error = nil;
    
    id<MTLLibrary> library = [mDevice newLibraryWithSource:[NSString stringWithUTF8String:pShaderSource] options:nil error:&error];
    if (error)
    {
        //NSString* errorStr = error.domain;
        NSLog(@"metal shader error %@", [error localizedDescription]);
        return nullptr;
    }
    
    mLibrary = library;
    NSString *strName = [NSString stringWithUTF8String:getFunctionName(shaderStage)];
    mFunction = [library newFunctionWithName:strName];
    
    mShaderStage = shaderStage;
    return shared_from_this();
}

ShaderStage MTLShaderFunction::getShaderStage() const
{
    return mShaderStage;
}

id<MTLFunction> MTLShaderFunction::getShaderFunction() const
{
    return mFunction;
}

NAMESPACE_RENDERCORE_END
