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

const std::string& ShaderAsset::GetCompiledVertexShader() const
{
    RenderDevicePtr renderDevice = getRenderDevice();
    if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::GLES)
    {
        return mCompiledShaderString.gles30Shader.vertexShaderStr;
    }
    
    else if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        return mCompiledShaderString.metalShader.vertexShaderStr;
    }
    
    return "";
}

const std::string& ShaderAsset::GetCompiledFragmentShader() const
{
    RenderDevicePtr renderDevice = getRenderDevice();
    if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::GLES)
    {
        return mCompiledShaderString.gles30Shader.fragmentShaderStr;
    }
    
    else if (renderDevice && renderDevice->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        return mCompiledShaderString.metalShader.fragmentShaderStr;
    }
    
    return "";
}

bool ShaderAsset::LoadFromFile(const std::string& fileName)
{
    ShaderCodePtr hlshCodePtr1 = shader_compiler::compileHLSLToSPIRV(fileName.c_str(), ShaderStage_Vertex);
    if (hlshCodePtr1)
    {
        mCompiledShaderString.gles30Shader.vertexShaderStr = shader_compiler::compileToESSL30(*hlshCodePtr1, ShaderStage_Vertex);
        mCompiledShaderString.metalShader.vertexShaderStr = shader_compiler::compileToMSL(*hlshCodePtr1, ShaderStage_Vertex).shaderSource;
    }
    
    ShaderCodePtr hlshCodePtr2 = shader_compiler::compileHLSLToSPIRV(fileName.c_str(), ShaderStage_Fragment);
    if (hlshCodePtr2)
    {
        mCompiledShaderString.gles30Shader.fragmentShaderStr = shader_compiler::compileToESSL30(*hlshCodePtr2, ShaderStage_Fragment);
        mCompiledShaderString.metalShader.fragmentShaderStr = shader_compiler::compileToMSL(*hlshCodePtr2, ShaderStage_Fragment).shaderSource;
    }
    return true;
}

NS_RENDERSYSTEM_END
