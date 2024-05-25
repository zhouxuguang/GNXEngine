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
    
//    NSString* shaderNameNS = [NSString stringWithUTF8String:shaderName.c_str()];
//    NSString *shadePath = [[NSBundle mainBundle] pathForResource:shaderNameNS ofType:@"hlsl"];
    
    // /Users/zhouxuguang/work/mycode/GNXEngine/source/shader/built-in
    std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".hlsl";
    
    ShaderCodePtr hlshCodePtr1 = shader_compiler::compileHLSLToSPIRV(shaderFilePath.c_str(), ShaderStage_Vertex);
    if (hlshCodePtr1)
    {
        shaderAssetString.gles30Shader.vertexShaderStr = shader_compiler::compileToESSL30(*hlshCodePtr1, ShaderStage_Vertex);
        
        CompiledShaderInfo shaderInfo = shader_compiler::compileToMSL(*hlshCodePtr1, ShaderStage_Vertex);
        shaderAssetString.metalShader.vertexShaderStr = shaderInfo.shaderSource;
        shaderAssetString.vertexDescriptor = shaderInfo.vertexDescriptor;
        shaderAssetString.vertexUniformBufferLayout = std::move(shaderInfo.vertexUniformBufferLayout);
    }
    
    ShaderCodePtr hlshCodePtr2 = shader_compiler::compileHLSLToSPIRV(shaderFilePath.c_str(), ShaderStage_Fragment);
    if (hlshCodePtr2)
    {
        shaderAssetString.gles30Shader.fragmentShaderStr = shader_compiler::compileToESSL30(*hlshCodePtr2, ShaderStage_Fragment);
        
        CompiledShaderInfo shaderInfo = shader_compiler::compileToMSL(*hlshCodePtr2, ShaderStage_Fragment);
        shaderAssetString.metalShader.fragmentShaderStr = shaderInfo.shaderSource;
        shaderAssetString.fragmentUniformBufferLayout = std::move(shaderInfo.fragmentUniformBufferLayout);
        //shaderAssetString.vertexDescriptor = shaderInfo.vertexDescriptor;
        
        //shaderAssetString.metalShader.fragmentShaderStr = shader_compiler::compileToMSL(*hlshCodePtr2, ShaderStage_Fragment);
    }
    
    ShaderCodePtr hlshCodePtr3 = shader_compiler::compileHLSLToSPIRV(shaderFilePath.c_str(), ShaderStage_Compute);
    if (hlshCodePtr3)
    {
        shaderAssetString.gles30Shader.computeShaderStr = shader_compiler::compileToESSL30(*hlshCodePtr3, ShaderStage_Compute);
        
        CompiledShaderInfo shaderInfo = shader_compiler::compileToMSL(*hlshCodePtr3, ShaderStage_Compute);
        shaderAssetString.metalShader.computeShaderStr = shaderInfo.shaderSource;
        shaderAssetString.fragmentUniformBufferLayout = std::move(shaderInfo.fragmentUniformBufferLayout);
        //shaderAssetString.vertexDescriptor = shaderInfo.vertexDescriptor;
        
        //shaderAssetString.metalShader.fragmentShaderStr = shader_compiler::compileToMSL(*hlshCodePtr2, ShaderStage_Fragment);
    }
    
    return shaderAssetString;
}

NS_RENDERSYSTEM_END