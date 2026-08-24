//
//  ShaderStageData.h
//  GNXEngine
//
//  跨模块共享的 shader 编译/加载值类型。
//
//  用途：作为 AssetManager（ShaderAsset / ShaderPackageBuilder）与
//  ShaderCompiler（CompiledShaderInfo）/ RenderSystem（ShaderAssetLoader）
//  之间的数据交换类型。字段复用 RenderCore 已有类型（ShaderCode / VertexDesc），
//  与 CompiledShaderInfo 同构，避免跨层重复转换。
//
//  Created by zhouxuguang on 2026.
//

#ifndef GNX_ENGINE_SHADER_STAGE_DATA_INCLUDE
#define GNX_ENGINE_SHADER_STAGE_DATA_INCLUDE

#include "RenderDefine.h"
#include "RenderDescriptor.h"
#include "ShaderFunction.h"
#include <string>
#include <vector>

NAMESPACE_RENDERCORE_BEGIN

// 单个 push constant block 的元数据（从 SPIR-V 编译阶段收集）
// 从 ShaderCompiler::CompiledPushConstantInfo 移入 RenderCore，
// 使 AssetManager 不依赖 ShaderCompiler.h。
struct CompiledPushConstantInfo
{
    std::string name;     // cbuffer 名称
    uint32_t size = 0;    // padded_size（已对齐）
    uint32_t set = 0;     // 原始 descriptor set（用于按 index 绑定时的反向查表）
    uint32_t binding = 0; // 原始 descriptor binding
};

// 单个 stage 的编译/加载结果（与 pb ShaderMessage 对应，但为纯 C++ 值类型）
struct ShaderStageData
{
    // ====== 标识 ======
    ShaderStage stage = ShaderStage_Vertex;   // VS / PS / CS / TS / MS
    ShaderFormat format = ShaderFormat_SPIRV; // 目标格式
    std::string entryPoint;                   // 入口函数名（Metal 需要）
    uint64_t sourceHash = 0;                  // HLSL 源码 hash（增量编译 / 版本追踪）

    // ====== 编译后的 shader 数据 ======
    ShaderCode sourceData;                    // 格式由 format 决定（MSL 文本 / SPIR-V 二进制）

    // ====== 反射元数据 ======
    RenderCore::VertexDesc vertexDescriptor;  // 顶点描述（Metal 重建 MTLVertexDescriptor 必需）
    std::vector<CompiledPushConstantInfo> pushConstants;  // push constant 布局（Vulkan 管线布局需要）

    // ====== Threadgroup 大小（TS / MS / CS 有效） ======
    uint32_t threadgroupSizeX = 0;
    uint32_t threadgroupSizeY = 0;
    uint32_t threadgroupSizeZ = 0;
};

NAMESPACE_RENDERCORE_END

#endif // GNX_ENGINE_SHADER_STAGE_DATA_INCLUDE
