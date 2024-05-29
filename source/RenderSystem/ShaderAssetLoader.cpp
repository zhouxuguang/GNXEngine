//
//  ShaderAssetLoader.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/12.
//

#include "ShaderAssetLoader.h"
#include "ShaderCompiler.h"
#include "RenderEngine.h"

using namespace shader_compiler;

NS_RENDERSYSTEM_BEGIN

ShaderAssetString LoadShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;
    
    // /Users/zhouxuguang/work/mycode/GNXEngine/source/shader/built-in
    std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".hlsl";
    
    ShaderCodePtr hlshCodePtr1 = shader_compiler::compileHLSLToSPIRV(shaderFilePath.c_str(), ShaderStage_Vertex);
    if (hlshCodePtr1)
    {
        shaderAssetString.gles30Shader.vertexShader = shader_compiler::compileToESSL30(hlshCodePtr1, ShaderStage_Vertex);
        
        CompiledShaderInfo shaderInfo = shader_compiler::compileToMSL(hlshCodePtr1, ShaderStage_Vertex);
        shaderAssetString.metalShader.vertexShader = shaderInfo.shaderSource;
        shaderAssetString.vertexDescriptor = shaderInfo.vertexDescriptor;
        shaderAssetString.vertexUniformBufferLayout = std::move(shaderInfo.vertexUniformBufferLayout);
    }
    
    ShaderCodePtr hlshCodePtr2 = shader_compiler::compileHLSLToSPIRV(shaderFilePath.c_str(), ShaderStage_Fragment);
    if (hlshCodePtr2)
    {
        shaderAssetString.gles30Shader.fragmentShader = shader_compiler::compileToESSL30(hlshCodePtr2, ShaderStage_Fragment);
        
        CompiledShaderInfo shaderInfo = shader_compiler::compileToMSL(hlshCodePtr2, ShaderStage_Fragment);
        shaderAssetString.metalShader.fragmentShader = shaderInfo.shaderSource;
        shaderAssetString.fragmentUniformBufferLayout = std::move(shaderInfo.fragmentUniformBufferLayout);
        //shaderAssetString.vertexDescriptor = shaderInfo.vertexDescriptor;
        
        //shaderAssetString.metalShader.fragmentShaderStr = shader_compiler::compileToMSL(*hlshCodePtr2, ShaderStage_Fragment);
    }
    
    ShaderCodePtr hlshCodePtr3 = shader_compiler::compileHLSLToSPIRV(shaderFilePath.c_str(), ShaderStage_Compute);
    if (hlshCodePtr3)
    {
        shaderAssetString.gles30Shader.computeShader = shader_compiler::compileToESSL30(hlshCodePtr3, ShaderStage_Compute);
        
        CompiledShaderInfo shaderInfo = shader_compiler::compileToMSL(hlshCodePtr3, ShaderStage_Compute);
        shaderAssetString.metalShader.computeShader = shaderInfo.shaderSource;
        shaderAssetString.fragmentUniformBufferLayout = std::move(shaderInfo.fragmentUniformBufferLayout);
        //shaderAssetString.vertexDescriptor = shaderInfo.vertexDescriptor;
        
        //shaderAssetString.metalShader.fragmentShaderStr = shader_compiler::compileToMSL(*hlshCodePtr2, ShaderStage_Fragment);
    }
    
    return shaderAssetString;
}

NS_RENDERSYSTEM_END
