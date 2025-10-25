//
//  ShaderAsset.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#include "ShaderAsset.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "RenderCore/RenderDevice.h"

using namespace shader_compiler;

NS_RENDERSYSTEM_BEGIN

ShaderAsset::ShaderAsset()
{
    //
}

ShaderAsset::~ShaderAsset()
{
    //
}

#if 0

static ShaderCode defaultShader;

const ShaderCode& ShaderAsset::GetCompiledVertexShader() const
{
    RenderDevicePtr renderDevice = getRenderDevice();
    if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::GLES)
    {
        return mCompiledShaderString.gles30Shader.vertexShader;
    }
    
    else if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        return mCompiledShaderString.metalShader.vertexShader;
    }
    
    else if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::VULKAN)
    {
        return mCompiledShaderString.vulkanShader.vertexShader;
    }
    
    return defaultShader;
}

const ShaderCode& ShaderAsset::GetCompiledFragmentShader() const
{
    RenderDevicePtr renderDevice = getRenderDevice();
    if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::GLES)
    {
        return mCompiledShaderString.gles30Shader.fragmentShader;
    }
    
    else if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        return mCompiledShaderString.metalShader.fragmentShader;
    }
    
    else if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::VULKAN)
    {
        return mCompiledShaderString.vulkanShader.fragmentShader;
    }
    
    return defaultShader;
}

#endif

bool ShaderAsset::LoadFromFile(const std::string& fileName)
{
#if 0
    ShaderCodePtr hlshCodePtr1 = shader_compiler::compileHLSLToSPIRV(fileName.c_str(), ShaderStage_Vertex);
    //mCompiledShaderString.vulkanShader.vertexShader = *hlshCodePtr1;
    if (hlshCodePtr1)
    {
        //mCompiledShaderString.gles30Shader.vertexShader = shader_compiler::compileToESSL30(*hlshCodePtr1, ShaderStage_Vertex);
        //mCompiledShaderString.metalShader.vertexShader = shader_compiler::compileToMSL(*hlshCodePtr1, ShaderStage_Vertex).shaderSource;
    }
    
    ShaderCodePtr hlshCodePtr2 = shader_compiler::compileHLSLToSPIRV(fileName.c_str(), ShaderStage_Fragment);
    if (hlshCodePtr2)
    {
        //mCompiledShaderString.gles30Shader.fragmentShader = shader_compiler::compileToESSL30(*hlshCodePtr2, ShaderStage_Fragment);
        //mCompiledShaderString.metalShader.fragmentShader = shader_compiler::compileToMSL(*hlshCodePtr2, ShaderStage_Fragment).shaderSource;
    }
#endif
    return true;
}

NS_RENDERSYSTEM_END
