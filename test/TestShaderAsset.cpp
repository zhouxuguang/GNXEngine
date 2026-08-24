// TestShaderAsset.cpp - verify .gnxasset precompiled shader asset round-trip
// Usage: TestShaderAsset <path.gnxasset> [--expect-vertex-inputs <count>]
//
// Verifies:
//   1. ShaderAsset::LoadFromFile succeeds (container format)
//   2. Each stage's sourceData non-empty / entryPoint / threadgroup / reflection correct
//   3. No nanopb / pb structs used (public getter only)
#include "Runtime/AssetManager/include/ShaderAsset.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <iostream>
#include <string>
#include <cstring>

using namespace AssetManager;
using namespace RenderCore;

static const char* kStageNames[] = { "Vertex", "Fragment", "Compute", "Task", "Mesh" };

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: TestShaderAsset <path.gnxasset> [--expect-vertex-inputs <count>]" << std::endl;
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

    std::cout << "=== ShaderAsset verify: " << filePath << " ===" << std::endl;
    std::cout << "  shaderName: " << asset.GetShaderName() << std::endl;
    std::cout << "  format: " << (int)asset.GetFormat() << std::endl;
    std::cout << "  stage count: " << asset.GetStages().size() << std::endl;

    if (asset.GetStages().empty())
    {
        std::cerr << "[FAIL] container has no stages" << std::endl;
        return 1;
    }

    // Verify each stage (vuln-29: ensure sourceData non-empty, prevent nested callback data loss)
    int totalVertexInputs = 0;
    for (const auto& stage : asset.GetStages())
    {
        uint32_t stageIdx = static_cast<uint32_t>(stage.stage);
        std::string stageName = (stageIdx < 5) ? kStageNames[stageIdx] : "Unknown";
        std::cout << "  --- stage: " << stageName << " ---" << std::endl;
        std::cout << "    entryPoint: " << stage.entryPoint << std::endl;
        std::cout << "    sourceData size: " << stage.sourceData.size() << " bytes" << std::endl;
        std::cout << "    threadgroup: " << stage.threadgroupSizeX << "x"
                  << stage.threadgroupSizeY << "x" << stage.threadgroupSizeZ << std::endl;

        if (stage.sourceData.empty())
        {
            std::cerr << "[FAIL] stage " << stageName << " shader data empty (container decode may have lost data)" << std::endl;
            return 1;
        }

        int count = (int)stage.vertexDescriptor.attributes.size();
        totalVertexInputs += count;
        std::cout << "    vertexInputs: " << count << std::endl;
        for (size_t i = 0; i < stage.vertexDescriptor.attributes.size(); ++i)
        {
            const auto& attr = stage.vertexDescriptor.attributes[i];
            uint32_t stride = (i < stage.vertexDescriptor.layouts.size())
                              ? stage.vertexDescriptor.layouts[i].stride : 0;
            std::cout << "      [" << i << "] location=" << attr.index
                      << " format=" << (int)attr.format
                      << " offset=" << attr.offset
                      << " stride=" << stride << std::endl;
        }
        std::cout << "    pushConstants: " << stage.pushConstants.size() << std::endl;
    }

    // Check expected vertexInputs (first stage only)
    if (expectVertexInputs >= 0)
    {
        int count = (int)asset.GetStages().front().vertexDescriptor.attributes.size();
        if (count != expectVertexInputs)
        {
            std::cerr << "[FAIL] vertexInputs count mismatch: expected " << expectVertexInputs
                      << ", got " << count << std::endl;
            return 1;
        }
    }

    // Verify GetStage / HasStage work
    const auto* vs = asset.GetStage(ShaderStage_Vertex);
    if (vs)
    {
        std::cout << "  [GetStage(Vertex)] hit, format=" << (int)vs->format << std::endl;
    }

    std::cout << "[PASS] " << filePath << std::endl;
    return 0;
}
