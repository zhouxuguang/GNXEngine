//
//  HiZPass.cpp
//  GNXEngine
//
//  Hi-Z (Hierarchical Z-Buffer) 生成Pass实现
//
//  Created by zhouxuguang
//

#include "HiZPass.h"
#include "ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderCore/include/ComputeEncoder.h"
#include "Runtime/MathUtil/include/Vector2.h"
#include <cmath>
#include <algorithm>

USING_NS_RENDERCORE

NS_RENDERSYSTEM_BEGIN

struct cbHiZParams
{
    mathutil::Vector2i textureSize;          // 当前层级的纹理尺寸
    uint32_t mipLevel;                      // 当前生成的Mip Level
    uint32_t padding;
    
    mathutil::Vector2f invTextureSize;      // 1.0 / textureSize
    mathutil::Vector2f padding2;
};

HiZPass::HiZPass()
{
}

HiZPass::~HiZPass()
{
    FreeGPUResources();
}

bool HiZPass::Initialize()
{
    if (mInitialized)
    {
        return true;
    }
    
    // 创建Hi-Z生成Pipeline
    CreateHiZPipeline();
    
    mHiZParas = GetRenderDevice()->CreateUniformBufferWithSize(sizeof(cbHiZParams));
    
    mInitialized = true;
    return true;
}

HiZOutput HiZPass::AddToFrameGraph(
    const std::string& passName,
    FrameGraph& frameGraph,
    CommandBufferPtr commandBuffer,
    const HiZParams& params)
{
    mCommandBuffer = commandBuffer;
    
    // 如果尺寸改变，重新创建Hi-Z纹理
    if (mCurrentWidth != params.width || mCurrentHeight != params.height)
    {
        mCurrentWidth = params.width;
        mCurrentHeight = params.height;
    }
    
    // 计算层级数量
    mHiZLevels = CalculateNumLevels(mCurrentWidth / 2, mCurrentHeight / 2);
    
    // 定义Pass数据
    struct HiZPassData
    {
        FrameGraphResource depthInput;
        FrameGraphResource hiZOutput;
        uint32_t numLevels;
    };
    
    // 添加Compute Pass到FrameGraph
    const auto& data = frameGraph.AddPass<HiZPassData>(
        passName,
        // Setup阶段：声明资源读写
        [&](FrameGraph::Builder& builder, HiZPassData& passData) 
        {
            // 读取深度缓冲
            passData.depthInput = builder.Read(
                params.depthTexture,
                (uint32_t)ResourceAccessType::ShaderRead
            );
            
            // 创建Hi-Z纹理输出
            FrameGraphTexture::Desc hiZDesc;
            hiZDesc.SetName(passName + "_HiZ");
            hiZDesc.extent.offsetX = 0;
            hiZDesc.extent.offsetY = 0;
            hiZDesc.extent.width = params.width / 2;
            hiZDesc.extent.height = params.height / 2;
            hiZDesc.format = RenderCore::kTexFormatR32Float;
            hiZDesc.numMipLevels = mHiZLevels;
            hiZDesc.layers = 1;
            
            passData.hiZOutput = builder.Create<FrameGraphTexture>(passName + "_HiZ", hiZDesc);
            
            // 写入Hi-Z纹理
            passData.hiZOutput = builder.Write(
                passData.hiZOutput,
                (uint32_t)ResourceAccessType::ComputeShaderWrite
            );
            
            passData.numLevels = mHiZLevels;
            
            builder.SetSideEffect();
        },
        // Execute阶段：执行Compute Shader
        [=](const HiZPassData& passData, FrameGraphPassResources& resources, void* context) 
        {
            float debugColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};
            SCOPED_DEBUGMARKER_EVENT(commandBuffer, resources.GetPassName().c_str(), debugColor);
            
            // 获取资源
            auto depthTexture = resources.Get<FrameGraphTexture>(passData.depthInput).texture;
            auto hiZTexture = resources.Get<FrameGraphTexture>(passData.hiZOutput).texture;
            
            if (!depthTexture || !hiZTexture)
            {
                return;
            }
            
            // 创建Compute Encoder
            ComputeEncoderPtr computeEncoder = mCommandBuffer->CreateComputeEncoder();
            if (!computeEncoder)
            {
                return;
            }
            
            computeEncoder->SetComputePipeline(mHiZPipeline);
            
            // 生成每一层Hi-Z
            uint32_t levelWidth = params.width / 2;
            uint32_t levelHeight = params.height / 2;
            
            for (uint32_t level = 0; level < passData.numLevels; ++level)
            {
                // 设置输入纹理
                if (level == 0)
                {
                    // 第一层：从原始深度缓冲采样
                    computeEncoder->SetTexture("srcDepth", depthTexture, 0);
                }
                else
                {
                    // 后续层：从上一层的Hi-Z采样
                    computeEncoder->SetTexture("srcDepth", hiZTexture, level - 1);
                }
                
                // 设置输出纹理（当前层）
                computeEncoder->SetOutTexture("dstDepth", hiZTexture, level);
                
                // 设置相关参数
                cbHiZParams hiZParams;
                hiZParams.textureSize.x = levelWidth;
                hiZParams.textureSize.y = levelHeight;
                hiZParams.mipLevel = level;
                
                mHiZParas->SetData(&hiZParams, 0, sizeof(hiZParams));
                computeEncoder->SetUniformBuffer("HiZParams", mHiZParas);
                
                // 计算Dispatch大小（8x8线程组）
                uint32_t groupX = (levelWidth + 7) / 8;
                uint32_t groupY = (levelHeight + 7) / 8;
                
                // 执行Compute Shader
                computeEncoder->Dispatch(groupX, groupY, 1);
                
                // 内存屏障：确保当前层写入完成，才能被下一层读取
                // 注意：这里需要根据RHI的具体实现添加内存屏障
                // Vulkan: vkCmdPipelineBarrier
                // Metal: MTLBarrier
                
                mCommandBuffer->ResourceBarrier(hiZTexture, ResourceAccessType::ComputeShaderRead | ResourceAccessType::ComputeShaderRead);
                
                // 准备下一层
                levelWidth = std::max(levelWidth / 2, 1u);
                levelHeight = std::max(levelHeight / 2, 1u);
            }
            
            computeEncoder->EndEncode();
        }
    );
    
    HiZOutput output;
    output.hiZTexture = data.hiZOutput;
    output.numLevels = data.numLevels;
    
    return output;
}

void HiZPass::CreateHiZPipeline()
{
    // 加载Hi-Z生成Shader
    ShaderAssetString shaderAsset = LoadShaderAsset("HiZGeneration");
    
    // 使用加载的Shader
    mHiZPipeline = GetRenderDevice()->CreateComputePipeline(
        *shaderAsset.computeShader->shaderSource
    );
}

uint32_t HiZPass::CalculateNumLevels(uint32_t width, uint32_t height) const
{
    uint32_t levels = 0;
    uint32_t w = width / 2;
    uint32_t h = height / 2;
    
    // 持续降采样直到达到最小尺寸（2x2或更小）
    while (w >= 2 && h >= 2 && levels < kMaxHiZLevels)
    {
        levels++;
        w /= 2;
        h /= 2;
    }
    
    // 确保至少有一层
    return std::max(levels, 1u);
}

void HiZPass::FreeGPUResources()
{
    mHiZPipeline.reset();
    
    mHiZLevels = 0;
    mInitialized = false;
}

NS_RENDERSYSTEM_END
