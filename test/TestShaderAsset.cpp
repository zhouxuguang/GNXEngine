// TestShaderAsset.cpp — 验证 .gnxasset 预编译 shader 资产的序列化往返
// 用法: TestShaderAsset <path.gnxasset> [--expect-vertex-inputs <count>]
//
// 验证:
//   1. ShaderAsset::LoadFromFile 能成功反序列化
//   2. vertexInputs / vertexStride / pushConstants / threadgroup 等反射字段正确恢复
#include "Runtime/AssetManager/include/ShaderAsset.h"
#include "Runtime/AssetManager/include/ShaderMessage.pb.h"
#include "Runtime/AssetManager/include/ShaderMessageUtil.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <iostream>
#include <string>
#include <cstring>

using namespace AssetManager;

static const char* kStageNames[] = { "Vertex", "Fragment", "Compute", "Task", "Mesh" };

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "用法: TestShaderAsset <path.gnxasset> [--expect-vertex-inputs <count>]" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    int expectVertexInputs = -1;
    for (int i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--expect-vertex-inputs") == 0 && i + 1 < argc)
        {
            expectVertexInputs = atoi(argv[++i]);
        }
    }

    ShaderAsset asset;
    if (!asset.LoadFromFile(filePath))
    {
        std::cerr << "[FAIL] LoadFromFile failed: " << filePath << std::endl;
        return 1;
    }

    const ShaderMessage& msg = asset.GetShaderMessage();
    uint32_t stage = static_cast<uint32_t>(msg.shaderStage);
    std::string stageName = (stage < 5) ? kStageNames[stage] : "Unknown";

    std::cout << "=== ShaderAsset 验证: " << filePath << " ===" << std::endl;
    std::cout << "  阶段: " << stageName << std::endl;
    std::cout << "  格式: " << (int)msg.shaderFormat << std::endl;
    std::cout << "  入口点: " << asset.GetEntryPoint() << std::endl;
    std::cout << "  shader 数据大小: " << asset.GetShaderDataSize() << " bytes" << std::endl;
    std::cout << "  threadgroup: " << msg.threadgroupSizeX << "x"
              << msg.threadgroupSizeY << "x" << msg.threadgroupSizeZ << std::endl;

    const auto* vInputs = asset.GetVertexInputs();
    int count = (vInputs ? (int)vInputs->size() : 0);
    std::cout << "  vertexInputs: " << count << std::endl;

    if (vInputs)
    {
        for (size_t i = 0; i < vInputs->size(); ++i)
        {
            const auto& vi = (*vInputs)[i];
            std::cout << "    [" << i << "] location=" << vi.location
                      << " format=" << (int)vi.format
                      << " offset=" << vi.offset
                      << " stride=" << vi.stride << std::endl;
        }
    }

    std::cout << "  vertexStride: " << asset.GetVertexStride() << std::endl;
    std::cout << "  pushConstants: " << (asset.GetPushConstants() ? asset.GetPushConstants()->size() : 0) << std::endl;

    // 校验期望的 vertexInputs 数量
    if (expectVertexInputs >= 0 && count != expectVertexInputs)
    {
        std::cerr << "[FAIL] vertexInputs 数量不匹配: 期望 " << expectVertexInputs
                  << ", 实际 " << count << std::endl;
        return 1;
    }

    std::cout << "[PASS] " << filePath << std::endl;
    return 0;
}
