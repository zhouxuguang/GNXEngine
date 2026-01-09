#pragma once

#include "FrameGraph.h"
#include "FrameGraphTexture.h"
#include "FrameGraphBuffer.h"

NAMESPACE_GNXENGINE_BEGIN

/**
 * @brief 资源屏障使用示例
 * 
 * 这些示例展示了如何使用自动资源屏障系统
 */

/**
 * 示例1：简单的后处理 Pass
 */
void ExamplePostProcessingPass(FrameGraph& fg, FrameGraphResource srcTexture)
{
    struct PassData {
        FrameGraphResource input;
        FrameGraphResource output;
    };
    
    fg.AddPass("PostProcessing",
        [](FrameGraph::Builder& builder, PassData& data) {
            // 读取输入纹理（作为着色器资源）
            data.input = builder.Read(srcTexture, 
                static_cast<uint32_t>(ResourceAccessType::ShaderResource));
            
            // 创建输出纹理
            FrameGraphTexture::Desc outputDesc = {};
            outputDesc.extent.width = 1920;
            outputDesc.extent.height = 1080;
            outputDesc.format = RenderCore::kTexFormatR8G8B8A8_UNorm;
            
            data.output = builder.Create<FrameGraphTexture>("PostProcessOutput", outputDesc);
            
            // 写入输出纹理（作为颜色附件）
            builder.Write(data.output, 
                static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
        },
        [](const PassData& data, FrameGraphPassResources& resources, void* context) {
            // 系统已经自动插入屏障：
            // - srcTexture 从 ShaderReadOnly -> ShaderReadOnly（不需要屏障）
            // - output 从 Undefined -> ColorAttachmentOptimal（不需要屏障）
            
            auto& inputTex = resources.Get<FrameGraphTexture>(data.input);
            auto& outputTex = resources.Get<FrameGraphTexture>(data.output);
            
            // 执行后处理渲染
            VkCommandBuffer cmd = static_cast<VkCommandBuffer>(context);
            // ... 渲染代码 ...
        }
    );
}

/**
 * 示例2：计算着色器到图形 Pass 的数据流
 */
void ExampleComputeToGraphicsPass(FrameGraph& fg, FrameGraphResource inputTexture)
{
    struct ComputeData {
        FrameGraphResource input;
        FrameGraphResource computeOutput;
    };
    
    struct GraphicsData {
        FrameGraphResource input;
        FrameGraphResource output;
    };
    
    // 计算着色器 Pass
    fg.AddPass("ComputePass",
        [](FrameGraph::Builder& builder, ComputeData& data) {
            // 读取输入纹理
            data.input = builder.Read(inputTexture, 
                static_cast<uint32_t>(ResourceAccessType::ComputeShaderResource));
            
            // 创建计算输出纹理
            FrameGraphTexture::Desc outputDesc = {};
            outputDesc.extent.width = 1920;
            outputDesc.extent.height = 1080;
            outputDesc.format = RenderCore::kTexFormatR8G8B8A8_UNorm;
            
            data.computeOutput = builder.Create<FrameGraphTexture>("ComputeOutput", outputDesc);
            
            // 写入输出纹理（作为存储图像）
            builder.Write(data.computeOutput, 
                static_cast<uint32_t>(ResourceAccessType::StorageBufferWrite));
        },
        [](const ComputeData& data, FrameGraphPassResources& resources, void* context) {
            auto& inputTex = resources.Get<FrameGraphTexture>(data.input);
            auto& outputTex = resources.Get<FrameGraphTexture>(data.computeOutput);
            
            // 执行计算着色器
            VkCommandBuffer cmd = static_cast<VkCommandBuffer>(context);
            // ... 计算着色器代码 ...
        }
    );
    
    // 图形 Pass
    fg.AddPass("GraphicsPass",
        [](FrameGraph::Builder& builder, GraphicsData& data) {
            // 读取计算着色器的输出
            // 系统会自动插入屏障：General -> ShaderReadOnly
            data.input = builder.Read(computeOutput, 
                static_cast<uint32_t>(ResourceAccessType::ShaderResource));
            
            // 创建最终输出
            FrameGraphTexture::Desc outputDesc = {};
            outputDesc.extent.width = 1920;
            outputDesc.extent.height = 1080;
            outputDesc.format = RenderCore::kTexFormatR8G8B8A8_UNorm;
            
            data.output = builder.Create<FrameGraphTexture>("FinalOutput", outputDesc);
            
            // 写入输出纹理
            builder.Write(data.output, 
                static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
        },
        [](const GraphicsData& data, FrameGraphPassResources& resources, void* context) {
            auto& inputTex = resources.Get<FrameGraphTexture>(data.input);
            auto& outputTex = resources.Get<FrameGraphTexture>(data.output);
            
            // 执行图形渲染
            VkCommandBuffer cmd = static_cast<VkCommandBuffer>(context);
            // ... 渲染代码 ...
        }
    );
}

/**
 * 示例3：延迟渲染
 */
void ExampleDeferredRendering(FrameGraph& fg)
{
    struct GBufferData {
        FrameGraphResource albedo;
        FrameGraphResource normal;
        FrameGraphResource depth;
    };
    
    struct LightingData {
        FrameGraphResource albedo;
        FrameGraphResource normal;
        FrameGraphResource depth;
        FrameGraphResource lighting;
    };
    
    // 几何 Pass
    fg.AddPass("GeometryPass",
        [](FrameGraph::Builder& builder, GBufferData& data) {
            // 创建 G-Buffer 纹理
            FrameGraphTexture::Desc texDesc = {};
            texDesc.extent.width = 1920;
            texDesc.extent.height = 1080;
            texDesc.format = RenderCore::kTexFormatR8G8B8A8_UNorm;
            
            data.albedo = builder.Create<FrameGraphTexture>("Albedo", texDesc);
            data.normal = builder.Create<FrameGraphTexture>("Normal", texDesc);
            
            texDesc.format = RenderCore::kTexFormatD24_UNorm_S8_UInt;
            data.depth = builder.Create<FrameGraphTexture>("Depth", texDesc);
            
            // 写入所有 G-Buffer 纹理
            builder.Write(data.albedo, 
                static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
            builder.Write(data.normal, 
                static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
            builder.Write(data.depth, 
                static_cast<uint32_t>(ResourceAccessType::DepthStencilAttachment));
        },
        [](const GBufferData& data, FrameGraphPassResources& resources, void* context) {
            VkCommandBuffer cmd = static_cast<VkCommandBuffer>(context);
            // 渲染几何体到 G-Buffer
        }
    );
    
    // 光照 Pass
    fg.AddPass("LightingPass",
        [](FrameGraph::Builder& builder, LightingData& data) {
            // 读取 G-Buffer
            data.albedo = builder.Read(albedo, 
                static_cast<uint32_t>(ResourceAccessType::ShaderResource));
            data.normal = builder.Read(normal, 
                static_cast<uint32_t>(ResourceAccessType::ShaderResource));
            data.depth = builder.Read(depth, 
                static_cast<uint32_t>(ResourceAccessType::ShaderResource));
            
            // 创建光照输出
            FrameGraphTexture::Desc outputDesc = {};
            outputDesc.extent.width = 1920;
            outputDesc.extent.height = 1080;
            outputDesc.format = RenderCore::kTexFormatR8G8B8A8_UNorm;
            
            data.lighting = builder.Create<FrameGraphTexture>("Lighting", outputDesc);
            
            // 写入光照结果
            builder.Write(data.lighting, 
                static_cast<uint32_t>(ResourceAccessType::ColorAttachment));
        },
        [](const LightingData& data, FrameGraphPassResources& resources, void* context) {
            VkCommandBuffer cmd = static_cast<VkCommandBuffer>(context);
            // 执行光照计算
        }
    );
}

/**
 * 示例4：使用缓冲区进行计算
 */
void ExampleComputeBufferPass(FrameGraph& fg, FrameGraphResource inputData)
{
    struct PassData {
        FrameGraphResource inputBuffer;
        FrameGraphResource outputBuffer;
    };
    
    fg.AddPass("ComputeBufferPass",
        [](FrameGraph::Builder& builder, PassData& data) {
            // 读取输入缓冲区
            data.inputBuffer = builder.Read(inputData, 
                static_cast<uint32_t>(ResourceAccessType::StorageBufferRead));
            
            // 创建输出缓冲区
            FrameGraphBuffer::Desc bufferDesc = {};
            bufferDesc.size = 1024 * 1024; // 1MB
            
            data.outputBuffer = builder.Create<FrameGraphBuffer>("OutputBuffer", bufferDesc);
            
            // 写入输出缓冲区
            builder.Write(data.outputBuffer, 
                static_cast<uint32_t>(ResourceAccessType::StorageBufferWrite));
        },
        [](const PassData& data, FrameGraphPassResources& resources, void* context) {
            auto& inputBuf = resources.Get<FrameGraphBuffer>(data.inputBuffer);
            auto& outputBuf = resources.Get<FrameGraphBuffer>(data.outputBuffer);
            
            VkCommandBuffer cmd = static_cast<VkCommandBuffer>(context);
            // ... 计算着色器代码 ...
        }
    );
}

/**
 * 示例5：混合使用异步计算
 */
void ExampleAsyncComputePass(FrameGraph& fg, FrameGraphResource texture)
{
    struct ComputeData {
        FrameGraphResource input;
        FrameGraphResource output;
    };
    
    // 计算着色器 Pass（异步）
    fg.AddPass("AsyncComputePass",
        [](FrameGraph::Builder& builder, ComputeData& data) {
            builder.EnableAsyncCompute(true);
            
            data.input = builder.Read(texture, 
                static_cast<uint32_t>(ResourceAccessType::ComputeShaderResource));
            
            FrameGraphTexture::Desc outputDesc = {};
            outputDesc.extent.width = 1920;
            outputDesc.extent.height = 1080;
            outputDesc.format = RenderCore::kTexFormatR8G8B8A8_UNorm;
            
            data.output = builder.Create<FrameGraphTexture>("AsyncOutput", outputDesc);
            builder.Write(data.output, 
                static_cast<uint32_t>(ResourceAccessType::StorageBufferWrite));
        },
        [](const ComputeData& data, FrameGraphPassResources& resources, void* context) {
            VkCommandBuffer cmd = static_cast<VkCommandBuffer>(context);
            // 异步计算着色器代码
        }
    );
}

NAMESPACE_GNXENGINE_END
