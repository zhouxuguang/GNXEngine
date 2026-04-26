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

        case ShaderStage_Task:
            return "AS";

        case ShaderStage_Mesh:
            return "MS";

        default:
            break;
    }

    return "";
}

ShaderFunctionPtr MTLShaderFunction::InitWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage)
{
    if (shaderSource.empty())
    {
        return nullptr;
    }
    
    NSError *error = nil;
    
    std::string shaderStr;
    shaderStr.resize(shaderSource.size());
    memcpy(shaderStr.data(), shaderSource.data(), shaderSource.size());

    MTLCompileOptions *options = [MTLCompileOptions new];
    if (@available(macOS 13.0, iOS 16.0, *))
    {
        options.languageVersion = MTLLanguageVersion3_0;
    }

    id<MTLLibrary> library = [mDevice newLibraryWithSource:[NSString stringWithUTF8String:shaderStr.c_str()] options:options error:&error];
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

ShaderStage MTLShaderFunction::GetShaderStage() const
{
    return mShaderStage;
}

id<MTLFunction> MTLShaderFunction::GetShaderFunction() const
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

    MTLCompileOptions *options = [MTLCompileOptions new];
    if (@available(macOS 13.0, iOS 16.0, *))
    {
        options.languageVersion = MTLLanguageVersion3_0;
    }

    NSError *error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:shaderStr.c_str()] options:options error:&error];
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
    
#if SUPPORTED_NEW_REFLECT
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
#else
    for (MTLArgument * arg in reflectionObj.vertexArguments)
    {
        NSLog(@"MTLGraphicsShader::GenerateRefectionInfo Found arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
        
        mVertexBindings[arg.name.UTF8String] = arg.index;
    }
    
    for (MTLArgument * arg in reflectionObj.fragmentArguments)
    {
        NSLog(@"MTLGraphicsShader::GenerateRefectionInfo Found arg: %@, index = %lu\n", arg.name, (unsigned long)arg.index);
        
        mFragmentBindings[arg.name.UTF8String] = arg.index;
    }
#endif
}

NAMESPACE_RENDERCORE_END
