//
//  ShaderCompiler.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/9.
//

#include "ShaderCompiler.h"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_msl.hpp"

#include <map>
#include <utility>
#include "spirv_reflection.h"
#include "Runtime/BaseLib/include/PreCompile.h"
#include "Runtime/BaseLib/include/LogService.h"
// DXC: HLSL -> SPIR-V，Windows/macOS/Linux 的离线编译都需要
#if (GNX_OS_WINDOWS || GNX_OS_LINUX || GNX_OS_MACOS)
#include "DXCompilerUtil.h"
#endif
// DX12 链路专用：SPIR-V -> HLSL
#if GNX_OS_WINDOWS
#include "spirv_cross/spirv_hlsl.hpp"
#endif
#include "ReflectionInfo.h"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <vector>

NAMESPACE_SHADERCOMPILER_BEGIN

// ShaderCompilerConfig 静态成员定义
// 注意：UseReverseZ 的值由上层 RenderSystem 的 BuildSetting 在初始化时同步
bool ShaderCompilerConfig::UseReverseZ = true;

bool startsWith(const std::string& str, const std::string& prefix)
{
    return (str.rfind(prefix, 0) == 0);
}

//glslang使用说明
//https://stackoverflow.com/questions/38234986/how-to-use-glslang

ShaderCode compileToESSL30(ShaderCodePtr spirvCode, ShaderStage shaderStage)
{
    spirv_cross::CompilerGLSL glsl((const uint32_t *)spirvCode->data(), spirvCode->size() / 4);

    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    // Get all sampled images in the shader.
    for (auto &resource : resources.sampled_images)
    {
        unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);

        // Modify the decoration to prepare it for GLSL.
        glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

        // Some arbitrary remapping if we want.
        glsl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
    }
    
    // 对顶点输出和着色输入进行改装
    if (ShaderStage_Vertex == shaderStage)
    {
        for (auto &resource : resources.stage_outputs)
        {
            std::string outName = resource.name;
            if (startsWith(outName, "out."))
            {
                glsl.set_name(resource.id, outName.substr(4));
            }
        }
    }
    
    if (ShaderStage_Fragment == shaderStage)
    {
        for (auto &resource : resources.stage_inputs)
        {
            std::string inName = resource.name;
            if (startsWith(inName, "in."))
            {
                glsl.set_name(resource.id, inName.substr(3));
            }
        }
    }
    
    //处理纹理和采样器
    // Builds a mapping for all combinations of images and samplers.
    glsl.build_combined_image_samplers();

    // Give the remapped combined samplers new names.
    // Here you can also set up decorations if you want (binding = #N).
    for (auto &remap : glsl.get_combined_image_samplers())
    {
        glsl.set_name(remap.combined_id, spirv_cross::join("SPIRV_Cross_Combined", glsl.get_name(remap.image_id),
                                              glsl.get_name(remap.sampler_id)));
    }
    
    // Set some options.
    spirv_cross::CompilerGLSL::Options options;
    options.version = 300;
    options.es = true;
    glsl.set_common_options(options);

    // Compile to GLSL, ready to give to GL driver.
    std::string shaderStr = glsl.compile();
    ShaderCode shaderCode;
    shaderCode.resize(shaderStr.size());
    memcpy(shaderCode.data(), shaderStr.data(), shaderStr.size());
    
    return shaderCode;
}

// DXC Bug Workaround: DXC uses Workgroup storage class for the groupshared
// task payload variable in mesh shaders. According to SPV_EXT_mesh_shader,
// it should be TaskPayloadWorkgroupEXT. DXC correctly handles this in task
// shaders but misses the conversion in mesh shaders.
// Without this fix:
//   - SPIRV-Cross (Metal) translates the payload as "threadgroup" (uninitialized
//     local) instead of "const object_data" (Metal's task→mesh payload mechanism).
//   - Vulkan drivers receive invalid SPIR-V that violates the spec (mesh shader
//     payload must use TaskPayloadWorkgroupEXT, not Workgroup).
// See: https://github.com/microsoft/DirectXShaderCompiler/issues/5981
static void patchDXCMeshShaderPayloadBug(ShaderCodePtr spirvCode, ShaderStage shaderStage)
{
    if (shaderStage != ShaderStage_Mesh || !spirvCode || spirvCode->size() < 20)
        return;

    uint32_t* words = reinterpret_cast<uint32_t*>(spirvCode->data());
    size_t wordCount = spirvCode->size() / 4;
    size_t offset = 5; // skip 5-word SPIR-V header
    while (offset + 3 < wordCount)
    {
        uint32_t instr = words[offset];
        uint16_t opcode = instr & 0xFFFF;
        uint16_t wordCountInstr = (instr >> 16) & 0xFFFF;
        if (wordCountInstr == 0) break;

        if (opcode == 59 && wordCountInstr >= 4) // OpVariable
        {
            uint32_t& storageClass = words[offset + 3];
            if (storageClass == spv::StorageClassWorkgroup)
                storageClass = spv::StorageClassTaskPayloadWorkgroupEXT;
        }
        else if (opcode == 32 && wordCountInstr >= 3) // OpTypePointer
        {
            uint32_t& storageClass = words[offset + 2];
            if (storageClass == spv::StorageClassWorkgroup)
                storageClass = spv::StorageClassTaskPayloadWorkgroupEXT;
        }
        offset += wordCountInstr;
    }
}

// DXC may omit the optional task payload operand required by SPIRV-Cross HLSL.
static void patchEmitMeshTasksPayloadOperand(ShaderCodePtr spirvCode, ShaderStage shaderStage)
{
    if (shaderStage != ShaderStage_Task || !spirvCode || spirvCode->size() < 20)
    {
        return;
    }

    const size_t wordCount = spirvCode->size() / 4;
    const uint32_t* words = reinterpret_cast<const uint32_t*>(spirvCode->data());

    constexpr uint16_t kOpVariable = 59;
    constexpr uint16_t kOpEmitMeshTasksEXT = 5294;

    auto instructionOp = [](uint32_t instr) { return (uint16_t)(instr & 0xFFFF); };
    auto instructionLen = [](uint32_t instr) { return (uint16_t)((instr >> 16) & 0xFFFF); };

    // 1) 找到 task payload 变量（TaskPayloadWorkgroupEXT 存储类的 OpVariable）
    uint32_t payloadVarId = 0;
    for (size_t off = 5; off < wordCount;)
    {
        const uint16_t len = instructionLen(words[off]);
        if (len == 0 || off + len > wordCount)
        {
            break;
        }
        if (instructionOp(words[off]) == kOpVariable && len >= 4 &&
            words[off + 3] == spv::StorageClassTaskPayloadWorkgroupEXT)
        {
            payloadVarId = words[off + 2];   // OpVariable: [result type, result id, storage class]
            break;
        }
        off += len;
    }

    if (payloadVarId == 0)
    {
        return;   // 没有 payload 接口，无需处理
    }

    // 2) 重建模块，给缺少 payload 操作数的 OpEmitMeshTasksEXT 补上它。
    //    SPIR-V 内部只有 id 引用（没有字节偏移），因此插入字并整体后移是安全的。
    std::vector<uint32_t> rebuilt;
    rebuilt.reserve(wordCount + 1);
    rebuilt.insert(rebuilt.end(), words, words + 5);   // 5 字头部

    bool patched = false;
    for (size_t off = 5; off < wordCount;)
    {
        const uint16_t len = instructionLen(words[off]);
        if (len == 0 || off + len > wordCount)
        {
            break;
        }

        if (instructionOp(words[off]) == kOpEmitMeshTasksEXT && len == 4)
        {
            rebuilt.push_back((5u << 16) | kOpEmitMeshTasksEXT);
            rebuilt.push_back(words[off + 1]);
            rebuilt.push_back(words[off + 2]);
            rebuilt.push_back(words[off + 3]);
            rebuilt.push_back(payloadVarId);
            patched = true;
        }
        else
        {
            rebuilt.insert(rebuilt.end(), words + off, words + off + len);
        }
        off += len;
    }

    if (!patched)
    {
        return;
    }

    spirvCode->resize(rebuilt.size() * 4);
    memcpy(spirvCode->data(), rebuilt.data(), rebuilt.size() * 4);

}

// Pad struct array elements so SPIR-V strides are representable in HLSL.
static void patchStructArrayStridePaddingForHLSL(ShaderCodePtr spirvCode)
{
    if (!spirvCode || spirvCode->size() < 20)
    {
        return;
    }

    const size_t wordCount = spirvCode->size() / 4;
    const uint32_t* words = reinterpret_cast<const uint32_t*>(spirvCode->data());

    constexpr uint16_t kOpTypeInt          = 21;
    constexpr uint16_t kOpTypeFloat        = 22;
    constexpr uint16_t kOpTypeVector       = 23;
    constexpr uint16_t kOpTypeMatrix       = 24;
    constexpr uint16_t kOpTypeArray        = 28;
    constexpr uint16_t kOpTypeStruct       = 30;
    constexpr uint16_t kOpConstant         = 43;
    constexpr uint16_t kOpDecorate         = 71;
    constexpr uint16_t kOpMemberDecorate   = 72;
    constexpr uint32_t kDecorationArrayStride = 6;
    constexpr uint32_t kDecorationOffset      = 35;

    auto instrOp  = [](uint32_t instr) { return (uint16_t)(instr & 0xFFFF); };
    auto instrLen = [](uint32_t instr) { return (uint16_t)((instr >> 16) & 0xFFFF); };

    std::unordered_map<uint32_t, uint32_t> scalarSizes;   // 类型 id → 标量/向量/矩阵字节数
    std::unordered_map<uint32_t, uint32_t> constantValues; // 常量 id → 值（数组长度）
    std::unordered_map<uint32_t, uint32_t> arrayElem;     // OpTypeArray id → 元素类型 id
    std::unordered_map<uint32_t, uint32_t> arrayLength;   // OpTypeArray id → 元素个数
    std::unordered_map<uint32_t, std::vector<uint32_t>> structMembers;  // OpTypeStruct id → 成员类型 id
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>> memberOffsets;  // struct → member → offset
    std::unordered_map<uint32_t, uint32_t> arrayStride;   // OpTypeArray id → 步长

    uint32_t float32TypeId = 0;

    for (size_t off = 5; off < wordCount;)
    {
        const uint16_t len = instrLen(words[off]);
        if (len == 0 || off + len > wordCount)
        {
            break;
        }

        const uint16_t op = instrOp(words[off]);
        const uint32_t* args = words + off + 1;

        if (op == kOpTypeFloat && len >= 3)
        {
            const uint32_t width = args[1];
            scalarSizes[args[0]] = width / 8;
            if (width == 32 && float32TypeId == 0)
            {
                float32TypeId = args[0];
            }
        }
        else if (op == kOpTypeInt && len >= 4)
        {
            scalarSizes[args[0]] = args[1] / 8;
        }
        else if (op == kOpTypeVector && len >= 4)
        {
            auto scalar = scalarSizes.find(args[1]);
            if (scalar != scalarSizes.end())
            {
                scalarSizes[args[0]] = scalar->second * args[2];
            }
        }
        else if (op == kOpTypeMatrix && len >= 4)
        {
            // 矩阵按「列数个列向量」占用（cbuffer 中列向量 16 字节对齐的紧凑排列）
            auto column = scalarSizes.find(args[1]);
            if (column != scalarSizes.end())
            {
                scalarSizes[args[0]] = column->second * args[2];
            }
        }
        else if (op == kOpTypeArray && len >= 4)
        {
            arrayElem[args[0]] = args[1];
            // 第 3 个操作数是长度常量 id，需要从 OpConstant 还原出实际元素个数
            auto length = constantValues.find(args[2]);
            if (length != constantValues.end())
            {
                arrayLength[args[0]] = length->second;
            }
        }
        else if (op == kOpConstant && len >= 4)
        {
            constantValues[args[1]] = args[2];
        }
        else if (op == kOpTypeStruct && len >= 2)
        {
            std::vector<uint32_t> members(args + 1, args + len - 1);
            structMembers[args[0]] = std::move(members);
        }
        else if (op == kOpDecorate && len >= 3 && args[1] == kDecorationArrayStride)
        {
            arrayStride[args[0]] = args[2];
        }
        else if (op == kOpMemberDecorate && len >= 4 && args[2] == kDecorationOffset)
        {
            memberOffsets[args[0]][args[1]] = args[3];
        }

        off += len;
    }

    if (structMembers.empty() || arrayStride.empty())
    {
        return;
    }

    // 递归求类型占用的字节数（用于推算结构体的自然大小）。
    // 只依赖「成员偏移 + 成员大小」，与 SPIRV-Cross 的 HLSL 布局计算口径一致。
    std::unordered_map<uint32_t, uint32_t> sizeCache;
    std::function<uint32_t(uint32_t)> typeSize = [&](uint32_t typeId) -> uint32_t
    {
        auto cached = sizeCache.find(typeId);
        if (cached != sizeCache.end())
        {
            return cached->second;
        }

        uint32_t size = 0;

        auto scalar = scalarSizes.find(typeId);
        if (scalar != scalarSizes.end())
        {
            size = scalar->second;
        }
        else
        {
            auto array = arrayElem.find(typeId);
            auto stride = arrayStride.find(typeId);
            auto length = arrayLength.find(typeId);
            if (array != arrayElem.end() && stride != arrayStride.end() &&
                length != arrayLength.end() && length->second > 0)
            {
                // 数组整体按「步长 × 元素个数」占用（每个元素都按步长排布）
                size = stride->second * length->second;
            }
            else
            {
                auto structure = structMembers.find(typeId);
                if (structure != structMembers.end())
                {
                    const auto offsets = memberOffsets.find(typeId);
                    for (uint32_t i = 0; i < (uint32_t)structure->second.size(); ++i)
                    {
                        uint32_t memberOffset = 0;
                        if (offsets != memberOffsets.end())
                        {
                            auto found = offsets->second.find(i);
                            if (found != offsets->second.end())
                            {
                                memberOffset = found->second;
                            }
                        }
                        const uint32_t end = memberOffset + typeSize(structure->second[i]);
                        if (end > size)
                        {
                            size = end;
                        }
                    }
                }
            }
        }

        sizeCache[typeId] = size;
        return size;
    };

    // 找出「元素是结构体、且步长大于结构体自然大小」的数组，给元素结构体补填充成员
    std::unordered_map<uint32_t, uint32_t> padCounts;   // 结构体 id → 追加的 float 个数
    struct PendingOffset { uint32_t structId; uint32_t memberIndex; uint32_t offset; };
    std::vector<PendingOffset> pendingOffsets;

    for (const auto& kv : arrayStride)
    {
        auto elem = arrayElem.find(kv.first);
        if (elem == arrayElem.end())
        {
            continue;
        }
        if (structMembers.find(elem->second) == structMembers.end())
        {
            continue;
        }

        const uint32_t structSize = typeSize(elem->second);
        const uint32_t stride = kv.second;
        if (structSize == 0 || stride <= structSize || ((stride - structSize) % 4) != 0)
        {
            continue;
        }

        auto& padCount = padCounts[elem->second];
        for (uint32_t offset = structSize; offset < stride; offset += 4)
        {
            PendingOffset pending;
            pending.structId = elem->second;
            pending.memberIndex =
                (uint32_t)structMembers[elem->second].size() + padCount;
            pending.offset = offset;
            pendingOffsets.push_back(pending);
            ++padCount;
        }
    }


    if (padCounts.empty() || float32TypeId == 0)
    {
        return;
    }

    std::unordered_map<uint32_t, std::vector<PendingOffset>> pendingByStruct;
    for (const PendingOffset& pending : pendingOffsets)
    {
        pendingByStruct[pending.structId].push_back(pending);
    }

    // 重建模块：加长目标 OpTypeStruct 的成员表，并紧跟其后写入新的成员偏移装饰。
    std::vector<uint32_t> rebuilt;
    rebuilt.reserve(wordCount + pendingOffsets.size() * 5 + padCounts.size() * 4 + 16);
    rebuilt.insert(rebuilt.end(), words, words + 5);

    for (size_t off = 5; off < wordCount;)
    {
        const uint16_t len = instrLen(words[off]);
        if (len == 0 || off + len > wordCount)
        {
            break;
        }

        const uint16_t op = instrOp(words[off]);
        const uint32_t* args = words + off + 1;

        if (op == kOpTypeStruct && len >= 2 && padCounts.count(args[0]) != 0)
        {
            const uint32_t structId = args[0];
            const uint32_t padCount = padCounts[structId];
            const uint32_t newLen = (uint32_t)len + padCount;

            rebuilt.push_back(((newLen & 0xFFFF) << 16) | kOpTypeStruct);
            rebuilt.insert(rebuilt.end(), args, args + len - 1);
            for (uint32_t i = 0; i < padCount; ++i)
            {
                rebuilt.push_back(float32TypeId);
            }

            // 新增成员的 Offset 装饰：紧跟类型声明之后，保证解析时类型已存在
            for (const PendingOffset& pending : pendingByStruct[structId])
            {
                rebuilt.push_back((5u << 16) | kOpMemberDecorate);
                rebuilt.push_back(pending.structId);
                rebuilt.push_back(pending.memberIndex);
                rebuilt.push_back(kDecorationOffset);
                rebuilt.push_back(pending.offset);
            }
        }
        else
        {
            rebuilt.insert(rebuilt.end(), words + off, words + off + len);
        }

        off += len;
    }

    spirvCode->resize(rebuilt.size() * 4);
    memcpy(spirvCode->data(), rebuilt.data(), rebuilt.size() * 4);

}

// 将 ≤256B 的 UBO (cbuffer) 改写为 push constant
// 在 SPIR-V 二进制层面做三件事：
//   1. OpVariable: StorageClass Uniform(2) → PushConstant(9)
//   2. OpTypePointer: StorageClass Uniform(2) → PushConstant(9)
//   3. OpDecorate DescriptorSet(34) / Binding(33) → 替换为 OpNop
// 返回转换后的 push constant 元数据列表
static std::vector<CompiledPushConstantInfo> patchUniformToPushConstant(
    ShaderCodePtr spirvCode, ShaderStage shaderStage)
{
    std::vector<CompiledPushConstantInfo> result;
    if (!spirvCode || spirvCode->size() < 20)
        return result;

    // 用 SPIRV-Reflect 枚举所有 descriptor binding，找出 ≤256B 的 UBO
    SpvReflectShaderModule module;
    SpvReflectResult reflectResult = spvReflectCreateShaderModule(
        spirvCode->size(), spirvCode->data(), &module);
    if (reflectResult != SPV_REFLECT_RESULT_SUCCESS)
        return result;

    uint32_t bindingCount = 0;
    spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
    std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
    spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());

    // 收集目标 UBO 的 SPIR-V ID
    struct TargetUBO {
        uint32_t spirv_id;
        std::string name;
        uint32_t size;
        uint32_t set;
        uint32_t binding;
    };
    std::vector<TargetUBO> targets;
    for (uint32_t i = 0; i < bindingCount; i++)
    {
        const auto* b = bindings[i];
        if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
            b->block.padded_size > 0 &&
            b->block.padded_size <= 0)   // TODO: 暂时禁用 PushConstant（设为 maxPushConstantsSize 时启用）
        {
            targets.push_back({b->spirv_id, b->name ? b->name : "", b->block.padded_size, b->set, b->binding});
        }
    }

    if (!targets.empty())
    {
        // 将目标 spirv_id 放入 set 方便快速查
        std::unordered_set<uint32_t> targetIds;
        // 同时记录 spirv_id -> name/size 映射（用于输出）
        std::unordered_map<uint32_t, TargetUBO> targetMap;
        for (auto& t : targets)
        {
            targetIds.insert(t.spirv_id);
            targetMap[t.spirv_id] = t;
        }

        // 记录 OpVariable 的 result_type (pointer type id)
        std::unordered_set<uint32_t> pointerTypeIds;

        uint32_t* words = reinterpret_cast<uint32_t*>(spirvCode->data());
        size_t wordCount = spirvCode->size() / 4;
        size_t offset = 5; // skip 5-word SPIR-V header

        // 第一遍：查找 OpVariable 和 OpDecorate
        while (offset + 3 < wordCount)
        {
            uint32_t instr = words[offset];
            uint16_t opcode = instr & 0xFFFF;
            uint16_t wordCountInstr = (instr >> 16) & 0xFFFF;
            if (wordCountInstr == 0) break;

            if (opcode == 59 && wordCountInstr >= 4) // OpVariable
            {
                // words[offset+1] = result_type (pointer), words[offset+2] = result_id, words[offset+3] = storage_class
                uint32_t varId = words[offset + 2];
                if (targetIds.count(varId))
                {
                    uint32_t& storageClass = words[offset + 3];
                    if (storageClass == spv::StorageClassUniform)
                    {
                        storageClass = spv::StorageClassPushConstant;
                        pointerTypeIds.insert(words[offset + 1]);
                    }
                }
            }
            else if (opcode == 71 && wordCountInstr >= 3) // OpDecorate
            {
                uint32_t targetId = words[offset + 1];
                if (targetIds.count(targetId))
                {
                    uint32_t decoration = words[offset + 2];
                    if (decoration == spv::DecorationDescriptorSet ||
                        decoration == spv::DecorationBinding)
                    {
                        // 保留 OpDecorate DescriptorSet/Binding 不动
                        // set/binding 装饰对 PushConstant 变量无意义，但保留它们
                        // 以便 vulkan 后端在内部扫描恢复原始 set/binding 信息
                        // （用于按 index 绑定的反向查表路由）
                    }
                }
            }

            offset += wordCountInstr;
        }

        // 第二遍：查找 OpTypePointer 修改 storage class
        if (!pointerTypeIds.empty())
        {
            offset = 5;
            while (offset + 3 < wordCount)
            {
                uint32_t instr = words[offset];
                uint16_t opcode = instr & 0xFFFF;
                uint16_t wordCountInstr = (instr >> 16) & 0xFFFF;
                if (wordCountInstr == 0) break;

                if (opcode == 32 && wordCountInstr >= 3) // OpTypePointer
                {
                    // words[offset+1] = result_id, words[offset+2] = storage_class, words[offset+3] = pointed_type
                    uint32_t ptrId = words[offset + 1];
                    if (pointerTypeIds.count(ptrId))
                    {
                        uint32_t& storageClass = words[offset + 2];
                        if (storageClass == spv::StorageClassUniform)
                        {
                            storageClass = spv::StorageClassPushConstant;
                        }
                    }
                }

                offset += wordCountInstr;
            }
        }

        // 构造输出元数据
        for (auto& t : targets)
        {
            result.push_back({t.name, t.size, t.set, t.binding});
        }
    }

    spvReflectDestroyShaderModule(&module);
    return result;
}

CompiledShaderInfoPtr compileToMSL(ShaderCodePtr spirvCode, ShaderStage shaderStage, RenderCore::ShaderFormat targetFormat)
{
    spirv_cross::CompilerMSL msl((const uint32_t*)spirvCode->data(), spirvCode->size() / 4);

    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = msl.get_shader_resources();

    spv::ExecutionModel model;
    switch (shaderStage)
    {
        case ShaderStage_Vertex:  model = spv::ExecutionModelVertex; break;
        case ShaderStage_Fragment: model = spv::ExecutionModelFragment; break;
        case ShaderStage_Compute: model = spv::ExecutionModelGLCompute; break;
        case ShaderStage_Task:  model = spv::ExecutionModelTaskEXT; break;
        case ShaderStage_Mesh:  model = spv::ExecutionModelMeshEXT; break;
        default: model = spv::ExecutionModelVertex; break;
    }

    // 紧凑 sampled images 的 texture 和 sampler binding 到 0~N 连续索引
    // Metal 要求 sampler 索引必须在 0~15 范围内
    uint32_t nextTexBinding = 0;
    uint32_t nextSamBinding = 0;
    for (auto &resource : resources.sampled_images)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Image %s at set = %u, binding = %u -> MSL texture=%u, sampler=%u\n",
               resource.name.c_str(), set, binding, nextTexBinding, nextSamBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_texture = nextTexBinding;
        resBinding.msl_sampler = nextSamBinding;
        msl.add_msl_resource_binding(resBinding);

        nextTexBinding++;
        nextSamBinding++;
    }

    // 紧凑 separate samplers 的 binding（如果有独立的 sampler 变量）
    for (auto &resource : resources.separate_samplers)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Separate sampler %s at set = %u, binding = %u -> MSL sampler=%u\n",
               resource.name.c_str(), set, binding, nextSamBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_sampler = nextSamBinding;
        msl.add_msl_resource_binding(resBinding);

        nextSamBinding++;
    }

    // 紧凑 separate images 的 binding（如果有独立的 texture 变量）
    for (auto &resource : resources.separate_images)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Separate image %s at set = %u, binding = %u -> MSL texture=%u\n",
               resource.name.c_str(), set, binding, nextTexBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_texture = nextTexBinding;
        msl.add_msl_resource_binding(resBinding);

        nextTexBinding++;
    }

    // 紧凑 storage images 的 binding（RWTexture2D 等读写纹理，接续 sampled/separate images 的索引）
    for (auto &resource : resources.storage_images)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Storage image %s at set = %u, binding = %u -> MSL texture=%u\n",
               resource.name.c_str(), set, binding, nextTexBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_texture = nextTexBinding;
        msl.add_msl_resource_binding(resBinding);

        nextTexBinding++;
    }

    // 计算顶点属性占用的最大 location，用于调整 uniform buffer 起始索引
    uint32_t maxLocation = 0;
    int attrCount = 0;
    if (shaderStage == ShaderStage_Vertex)
    {
        for (auto &resource : resources.stage_inputs)
        {
            uint32_t loc = msl.get_decoration(resource.id, spv::DecorationLocation);
            attrCount++;
            if (loc > maxLocation)
            {
                maxLocation = loc;
            }
        }
        if (attrCount > 0)
        {
            maxLocation += 1;
        }
    }

    // 紧凑 uniform buffer binding
    uint32_t nextBufBinding = 0;
    if (shaderStage == ShaderStage_Vertex)
    {
        // 顶点着色器：buffer 索引从顶点属性之后开始
        nextBufBinding = maxLocation;
    }
    for (auto &resource : resources.uniform_buffers)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("UBO %s at set = %u, binding = %u -> MSL buffer=%u\n",
               resource.name.c_str(), set, binding, nextBufBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_buffer = nextBufBinding;
        msl.add_msl_resource_binding(resBinding);

        nextBufBinding++;
    }

    // 紧凑 storage buffer binding（接续 uniform buffer 的索引）
    for (auto &resource : resources.storage_buffers)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("SSBO %s at set = %u, binding = %u -> MSL buffer=%u\n",
               resource.name.c_str(), set, binding, nextBufBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_buffer = nextBufBinding;
        msl.add_msl_resource_binding(resBinding);

        nextBufBinding++;
    }

    spirv_cross::CompilerMSL::Options options;
    // MSL platform is decided explicitly by target format (no more compile-time macro):
    // ShaderFormat_MSL_iOS -> iOS, ShaderFormat_MSL_macOS -> macOS
    options.platform = (targetFormat == RenderCore::ShaderFormat_MSL_iOS)
                           ? spirv_cross::CompilerMSL::Options::iOS
                           : spirv_cross::CompilerMSL::Options::macOS;

    // 关键：使用 MSLResourceBinding 代替 decoration binding
    options.enable_decoration_binding = false;
    options.msl_version = spirv_cross::CompilerMSL::Options::make_msl_version(3, 0);
    // iOS 上 Task/Mesh Shader 会用到 WaveActiveSum/WavePrefixSum 等 subgroup 运算。
    // 默认走 quadgroup 路径会因不支持而抛异常，MSL 3.0 已保证 SIMD-group 可用，故显式启用。
    options.ios_use_simdgroup_functions = true;
    msl.set_msl_options(options);

    // Compile to msl, ready to give to metal driver.
    std::string shaderSource = msl.compile();
    
    CompiledShaderInfoPtr shaderInfo = std::make_shared<CompiledShaderInfo>();
    shaderInfo->format = targetFormat;
    if (shaderStage == ShaderStage_Vertex)
    {
        shaderInfo->vertexDescriptor = GetMetalReflectionInfo(msl, resources);
        //shaderInfo.vertexUniformBufferLayout = GetMetalUniformReflectionInfo(msl, resources);
    }

    // 提取 mesh/task/compute shader 的 threadgroup 大小（来自 SPIR-V LocalSize）
    if (shaderStage == ShaderStage_Task || shaderStage == ShaderStage_Mesh || shaderStage == ShaderStage_Compute)
    {
        shaderInfo->threadgroupSizeX = msl.get_execution_mode_argument(spv::ExecutionModeLocalSize, 0);
        shaderInfo->threadgroupSizeY = msl.get_execution_mode_argument(spv::ExecutionModeLocalSize, 1);
        shaderInfo->threadgroupSizeZ = msl.get_execution_mode_argument(spv::ExecutionModeLocalSize, 2);
    }
    
    shaderInfo->shaderSource = std::make_shared<ShaderCode>();
    shaderInfo->shaderSource->resize(shaderSource.size());
    memcpy(shaderInfo->shaderSource->data(), shaderSource.data(), shaderSource.size());
    
    return shaderInfo;
}

#if GNX_OS_WINDOWS
// SPIR-V -> HLSL for the DXIL path. Preserve SPIR-V binding numbers.
static CompiledShaderInfoPtr compileToHLSL(ShaderCodePtr spirvCode, ShaderStage shaderStage)
{
    spirv_cross::CompilerHLSL hlsl((const uint32_t*)spirvCode->data(), spirvCode->size() / 4);
    spirv_cross::ShaderResources resources = hlsl.get_shader_resources();

    spv::ExecutionModel model = spv::ExecutionModelVertex;
    switch (shaderStage)
    {
        case ShaderStage_Vertex:   model = spv::ExecutionModelVertex;    break;
        case ShaderStage_Fragment: model = spv::ExecutionModelFragment;  break;
        case ShaderStage_Compute:  model = spv::ExecutionModelGLCompute; break;
        case ShaderStage_Task:     model = spv::ExecutionModelTaskEXT;   break;
        case ShaderStage_Mesh:     model = spv::ExecutionModelMeshEXT;   break;
        default: break;
    }

    // NonWritable distinguishes SRVs from UAVs in storage_buffers.
    enum BindClass { kBcv = 0, kSrv, kUav, kSampler };

    std::map<std::pair<uint32_t, uint32_t>, spirv_cross::HLSLResourceBinding> bindingMap;

    auto addBinding = [&](const spirv_cross::Resource& res, BindClass cls)
    {
        const uint32_t set     = hlsl.get_decoration(res.id, spv::DecorationDescriptorSet);
        const uint32_t binding = hlsl.get_decoration(res.id, spv::DecorationBinding);

        const auto key = std::make_pair(set, binding);
        auto iter = bindingMap.find(key);
        if (iter == bindingMap.end())
        {
            spirv_cross::HLSLResourceBinding resBinding = {};
            resBinding.stage    = model;
            resBinding.desc_set = set;
            resBinding.binding  = binding;
            iter = bindingMap.emplace(key, resBinding).first;
        }

        // 同一个 (set, binding) 可能同时承载 srv 与 sampler（combined image sampler），
        // 因此这里按位补充而不是覆盖。
        switch (cls)
        {
            case kBcv:     iter->second.cbv.register_binding     = binding; break;
            case kSrv:     iter->second.srv.register_binding     = binding; break;
            case kUav:     iter->second.uav.register_binding     = binding; break;
            case kSampler: iter->second.sampler.register_binding = binding; break;
        }

    };

    for (auto& r : resources.uniform_buffers)
    {
        addBinding(r, kBcv);
    }

    // 只读 / 可写的存储缓冲与存储图像需要分别落到 SRV / UAV
    for (auto& r : resources.storage_buffers)
    {
        const bool readOnly = hlsl.has_decoration(r.id, spv::DecorationNonWritable);
        addBinding(r, readOnly ? kSrv : kUav);
    }
    for (auto& r : resources.storage_images)
    {
        const bool readOnly = hlsl.has_decoration(r.id, spv::DecorationNonWritable);
        addBinding(r, readOnly ? kSrv : kUav);
    }

    for (auto& r : resources.separate_images)   addBinding(r, kSrv);
    for (auto& r : resources.separate_samplers) addBinding(r, kSampler);
    for (auto& r : resources.sampled_images)
    {
        addBinding(r, kSrv);
        addBinding(r, kSampler);
    }

    spirv_cross::CompilerHLSL::Options options;
    // SM 6.0：引擎的 shader 使用 StructuredBuffer / RWTexture 等 SM 5+ 特性
    options.shader_model = 60;
    // 关键：默认情况下 SPIRV-Cross 把入口点命名为 "main"（use_entry_point_name=false），
    // 而 SPIR-V 的入口点名字正是引擎传给 DXC 的 -E 参数（VS/PS/CS/TS/MS）。
    // 开启该选项后，生成的 HLSL 入口点就是 VS/PS/CS/TS/MS，
    // 与 DXCompilerUtil::GetHLSLEntryPoint 的 -E 参数保持一致。
    options.use_entry_point_name = true;
    hlsl.set_hlsl_options(options);

    // 顶点语义：SPIRV-Cross 默认输出 TEXCOORD<location>，
    // 与 DX12ShaderFunction 按 input register 反射语义的方式天然吻合。
    std::string hlslSource;
    try
    {
        hlslSource = hlsl.compile();
    }
    catch (const std::exception& ex)
    {
        // SPIRV-Cross 用异常报告不支持的构造（例如不支持的 execution model），
        // 必须捕获，否则会直接终止离线编译进程。
        LOG_ERROR("SPIRV-Cross HLSL conversion failed (stage=%d): %s",
                  (int)shaderStage, ex.what());
        return nullptr;
    }

    CompiledShaderInfoPtr info = std::make_shared<CompiledShaderInfo>();
    info->format = RenderCore::ShaderFormat_DXIL;

    // 顶点描述需要传给 PSO 的输入布局（slot / offset / format）
    if (shaderStage == ShaderStage_Vertex)
    {
        info->vertexDescriptor = GetMetalReflectionInfo(hlsl, resources);
    }

    if (shaderStage == ShaderStage_Task || shaderStage == ShaderStage_Mesh ||
        shaderStage == ShaderStage_Compute)
    {
        info->threadgroupSizeX = hlsl.get_execution_mode_argument(spv::ExecutionModeLocalSize, 0);
        info->threadgroupSizeY = hlsl.get_execution_mode_argument(spv::ExecutionModeLocalSize, 1);
        info->threadgroupSizeZ = hlsl.get_execution_mode_argument(spv::ExecutionModeLocalSize, 2);
    }

    info->shaderSource = std::make_shared<ShaderCode>();
    info->shaderSource->resize(hlslSource.size());
    memcpy(info->shaderSource->data(), hlslSource.data(), hlslSource.size());

    return info;
}
#endif  // GNX_OS_WINDOWS

//HLSL shader脚本字符串转换
ShaderCodePtr compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType)
{
#if (GNX_OS_WINDOWS || GNX_OS_LINUX || GNX_OS_MACOS)
    return DXCompilerUtil::GetInstance()->compileHLSLToSPIRV(shaderFile, shaderStage, renderType);
#else
    return nullptr;
#endif
}

CompiledShaderInfoPtr CompileShader(const std::string& shaderFile, ShaderStage shaderStage, RenderCore::ShaderFormat targetFormat)
{
    // Target format decides the compile pipeline:
    //   SPIRV: DXC HLSL -> SPIR-V
    //   MSL_*: DXC HLSL -> SPIR-V -> Spirv-Cross -> MSL (platform from parameter)
    //   GLSL : DXC HLSL -> SPIR-V -> Spirv-Cross -> GLSL (ES 3.0)
    //   DXIL : not implemented, returns explicit error
    RenderDeviceType renderType = (targetFormat == RenderCore::ShaderFormat_SPIRV ||
                                   targetFormat == RenderCore::ShaderFormat_GLSL ||
                                   targetFormat == RenderCore::ShaderFormat_DXIL)
                                      ? RenderDeviceType::VULKAN
                                      : RenderDeviceType::METAL;

    ShaderCodePtr shaderCode = compileHLSLToSPIRV(shaderFile, shaderStage, renderType);
    if (!shaderCode)
    {
        return nullptr;
    }

    // Fix DXC mesh shader payload storage class bug for both Metal and Vulkan
    patchDXCMeshShaderPayloadBug(shaderCode, shaderStage);

    // Fix DXC 省略 OpEmitMeshTasksEXT payload 操作数的问题：
    // Vulkan 不依赖它，但 SPIRV-Cross 的 HLSL 后端依赖它，缺了就无法生成 DispatchMesh。
    patchEmitMeshTasksPayloadOperand(shaderCode, shaderStage);

    // Fix 「std140 结构体数组步长 vs HLSL 自然布局」不一致导致 SPIRV-Cross 拒绝生成 HLSL：
    // 给这类结构体补足填充成员（字节布局不变），使 SPIR-V → HLSL 这一步可用。
    patchStructArrayStridePaddingForHLSL(shaderCode);

    switch (targetFormat)
    {
        case RenderCore::ShaderFormat_MSL_iOS:
        case RenderCore::ShaderFormat_MSL_macOS:
        {
            CompiledShaderInfoPtr result = compileToMSL(shaderCode, shaderStage, targetFormat);
            if (result)
                result->format = targetFormat;
            return result;
        }

        case RenderCore::ShaderFormat_GLSL:
        {
            CompiledShaderInfoPtr compileShader = std::make_shared<CompiledShaderInfo>();
            compileShader->format = targetFormat;
            ShaderCode glslCode = compileToESSL30(shaderCode, shaderStage);
            compileShader->shaderSource = std::make_shared<ShaderCode>();
            compileShader->shaderSource->resize(glslCode.size());
            memcpy(compileShader->shaderSource->data(), glslCode.data(), glslCode.size());
            return compileShader;
        }

        case RenderCore::ShaderFormat_DXIL:
        {
            // .shader(HLSL) -> SPIR-V -> HLSL -> DXIL.
#if GNX_OS_WINDOWS
            CompiledShaderInfoPtr compileShader = std::make_shared<CompiledShaderInfo>();
            compileShader->format = targetFormat;

            // 顶点描述与 threadgroup 大小统一从 SPIR-V 反射得到：
            // 两条链路都需要它们，且与最终用哪条链路无关。
            {
                spirv_cross::Compiler reflection((const uint32_t*)shaderCode->data(),
                                                 shaderCode->size() / 4);
                spirv_cross::ShaderResources res = reflection.get_shader_resources();

                if (shaderStage == ShaderStage_Vertex)
                {
                    compileShader->vertexDescriptor = GetMetalReflectionInfo(reflection, res);
                }
                if (shaderStage == ShaderStage_Task || shaderStage == ShaderStage_Mesh ||
                    shaderStage == ShaderStage_Compute)
                {
                    compileShader->threadgroupSizeX =
                        reflection.get_execution_mode_argument(spv::ExecutionModeLocalSize, 0);
                    compileShader->threadgroupSizeY =
                        reflection.get_execution_mode_argument(spv::ExecutionModeLocalSize, 1);
                    compileShader->threadgroupSizeZ =
                        reflection.get_execution_mode_argument(spv::ExecutionModeLocalSize, 2);
                }
            }

            CompiledShaderInfoPtr hlslInfo = compileToHLSL(shaderCode, shaderStage);
            if (!hlslInfo || !hlslInfo->shaderSource || hlslInfo->shaderSource->empty())
            {
                LOG_ERROR("CompileShader: SPIR-V to HLSL failed (stage=%d)", (int)shaderStage);
                return nullptr;
            }

            const std::string hlslText(hlslInfo->shaderSource->begin(),
                                       hlslInfo->shaderSource->end());
            ShaderCodePtr dxilCode =
                DXCompilerUtil::GetInstance()->compileHLSLTextToDXIL(hlslText, shaderStage);

            if (!dxilCode)
            {
                LOG_ERROR("CompileShader: HLSL to DXIL failed (stage=%d)", (int)shaderStage);
                return nullptr;
            }

            compileShader->shaderSource = dxilCode;
            if (!DXCompilerUtil::GetInstance()->reflectDXIL(
                    *dxilCode, shaderStage, compileShader->resources, compileShader->inputs))
            {
                LOG_ERROR("CompileShader: DXIL reflection metadata generation failed (stage=%d)",
                          (int)shaderStage);
                return nullptr;
            }
            // DX12 不使用 push constant（统一走 CBV），因此不填充 pushConstants
            return compileShader;
#else
            LOG_ERROR("CompileShader: DXIL format is only available on Windows");
            return nullptr;
#endif
        }

        case RenderCore::ShaderFormat_SPIRV:
        default:
        {
            CompiledShaderInfoPtr compileShader = std::make_shared<CompiledShaderInfo>();
            compileShader->format = targetFormat;
            compileShader->shaderSource = shaderCode;

            // 将 ≤256B 的 UBO 自动改写为 push constant（SPIR-V 二进制 patching）
            compileShader->pushConstants = patchUniformToPushConstant(shaderCode, shaderStage);

            // 提取 mesh/task/compute shader 的 threadgroup 大小
            if (shaderStage == ShaderStage_Task || shaderStage == ShaderStage_Mesh || shaderStage == ShaderStage_Compute)
            {
                spirv_cross::Compiler reflection((const uint32_t*)shaderCode->data(), shaderCode->size() / 4);
                compileShader->threadgroupSizeX = reflection.get_execution_mode_argument(spv::ExecutionModeLocalSize, 0);
                compileShader->threadgroupSizeY = reflection.get_execution_mode_argument(spv::ExecutionModeLocalSize, 1);
                compileShader->threadgroupSizeZ = reflection.get_execution_mode_argument(spv::ExecutionModeLocalSize, 2);
            }

            return compileShader;
        }
    }

    return nullptr;
}

NAMESPACE_SHADERCOMPILER_END
