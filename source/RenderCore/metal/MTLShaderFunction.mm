//
//  MTLShaderFunction.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#include "MTLShaderFunction.h"
#include <string>

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

ShaderFunctionPtr MTLShaderFunction::initWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    if (shaderSource.empty())
    {
        return nullptr;
    }
    
    NSError *error = nil;
    
    std::string shaderStr;
    shaderStr.resize(shaderSource.size());
    memcpy(shaderStr.data(), shaderSource.data(), shaderSource.size());
    
    id<MTLLibrary> library = [mDevice newLibraryWithSource:[NSString stringWithUTF8String:shaderStr.c_str()] options:nil error:&error];
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

#pragma region MTLGraphicsShader

static id<MTLFunction> CreateShaderFunction(id<MTLDevice> device, const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    if (shaderSource.empty())
    {
        return nil;
    }
    
    std::string shaderStr;
    shaderStr.resize(shaderSource.size());
    memcpy(shaderStr.data(), shaderSource.data(), shaderSource.size());
    
    NSError *error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:shaderStr.c_str()] options:nil error:&error];
    if (error)
    {
        //NSString* errorStr = error.domain;
        NSLog(@"metal shader error %@", [error localizedDescription]);
        library = nil;
        return nil;
    }
    
    NSString *strName = [NSString stringWithUTF8String:getFunctionName(shaderStage)];
    id<MTLFunction> function = [library newFunctionWithName:strName];
    library = nil;
    
    return function;
}

MTLGraphicsShader::MTLGraphicsShader(id<MTLDevice> device, const ShaderCode& vertexShader, const ShaderCode& fragmentShader)
{
    mVertexFunction = CreateShaderFunction(device, vertexShader, ShaderStage_Vertex);
    mFragmentFunction = CreateShaderFunction(device, fragmentShader, ShaderStage_Fragment);
}

void MTLGraphicsShader::GenerateRefectionInfo(MTLRenderPipelineReflection* reflectionObj)
{
    if (!reflectionObj)
    {
        return;
    }
    
    for (id<MTLBinding> arg in reflectionObj.vertexBindings)
    {
        NSLog(@"MTLGraphicsShader::GenerateRefectionInfo Found arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
        
        mVertexBindings[arg.name.UTF8String] = arg.index;
    }
    
    for (id<MTLBinding> arg in reflectionObj.fragmentBindings)
    {
        NSLog(@"MTLGraphicsShader::GenerateRefectionInfo Found arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
        
        mFragmentBindings[arg.name.UTF8String] = arg.index;
    }
}

NAMESPACE_RENDERCORE_END
