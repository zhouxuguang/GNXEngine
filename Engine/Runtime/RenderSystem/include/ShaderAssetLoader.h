//
//  ShaderAssetLoader.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/12.
//

#ifndef GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H
#define GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H

#include "RSDefine.h"
#include "Runtime/RenderCore/include/RenderDescriptor.h"
#include "Runtime/RenderCore/include/ShaderFunction.h"
#include "Runtime/RenderCore/include/ShaderStageData.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"

NS_RENDERSYSTEM_BEGIN

struct ShaderString
{
    ShaderCode vertexShader;
    ShaderCode fragmentShader;
    ShaderCode computeShader;
    ShaderCode taskShader;
    ShaderCode meshShader;
};

struct ShaderAssetString
{
    std::shared_ptr<RenderCore::ShaderStageData> vertexShader;
    std::shared_ptr<RenderCore::ShaderStageData> fragmentShader;
    std::shared_ptr<RenderCore::ShaderStageData> computeShader;
    std::shared_ptr<RenderCore::ShaderStageData> taskShader;
    std::shared_ptr<RenderCore::ShaderStageData> meshShader;

    VertexDesc vertexDescriptor;
};

struct GraphicsShaderInfo
{
    RenderCore::GraphicsPipelineDesc graphicsPipelineDesc;
    RenderCore::GraphicsShaderPtr graphicsShader;
};

RENDERSYSTEM_API ShaderAssetString LoadShaderAsset(const std::string &shaderName);

RENDERSYSTEM_API ShaderAssetString LoadCustomShaderAsset(const std::string &shaderName);

RENDERSYSTEM_API GraphicsShaderInfo CreateGraphicsShaderInfo(const std::string& shaderName);

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SHADER_ASSET_LOADER_INCLUDE_FKJBJBJ_H */
