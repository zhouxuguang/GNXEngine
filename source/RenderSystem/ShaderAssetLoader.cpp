//
//  ShaderAssetLoader.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/12.
//

#include "ShaderAssetLoader.h"
#include "ShaderCompiler.h"
#include "RenderEngine.h"
#include "RenderCore/RenderDevice.h"

using namespace shader_compiler;

NS_RENDERSYSTEM_BEGIN

ShaderAssetString LoadShaderAsset(const std::string &shaderName)
{
    ShaderAssetString shaderAssetString;
    
    std::string shaderFilePath = getBuiltInShaderDir() + shaderName + ".hlsl";
    
    RenderDeviceType renderType = getRenderDevice()->getRenderDeviceType();
    
    CompiledShaderInfoPtr vertexShaderInfo = CompileShader(shaderFilePath, ShaderStage_Vertex, renderType);
    
    if (vertexShaderInfo)
    {
        shaderAssetString.vertexShader = vertexShaderInfo;
        shaderAssetString.vertexDescriptor = vertexShaderInfo->vertexDescriptor;
    }
    
    CompiledShaderInfoPtr fragmentShaderInfo = CompileShader(shaderFilePath.c_str(), ShaderStage_Fragment, renderType);
    if (fragmentShaderInfo)
    {
        shaderAssetString.fragmentShader = fragmentShaderInfo;
    }
    
    CompiledShaderInfoPtr computeShaderInfo = CompileShader(shaderFilePath.c_str(), ShaderStage_Compute, renderType);
    if (computeShaderInfo)
    {
        shaderAssetString.computeShader = computeShaderInfo;
    }
    
    return shaderAssetString;
}

NS_RENDERSYSTEM_END
