//
//  VulkanPipelineCache.h
//  rendercore
//
//  Robust Vulkan Pipeline Cache with disk serialization.
//  Based on: https://zeux.io/2019/07/17/serializing-pipeline-cache/
//

#ifndef GNX_ENGINE_VK_PIPELINE_CACHE_INCLUDE_ADFGHJ
#define GNX_ENGINE_VK_PIPELINE_CACHE_INCLUDE_ADFGHJ

#include "VulkanContext.h"
#include "Runtime/BaseLib/include/LogService.h"

NAMESPACE_RENDERCORE_BEGIN

struct PipelineCachePrefixHeader
{
    uint32_t magic;         // Magic number to identify our cache file
    uint32_t dataSize;      // Size of pipeline cache data (from vkGetPipelineCacheData)
    uint64_t dataHash;      // Hash of pipeline cache data for integrity check

    uint32_t vendorID;      // VkPhysicalDeviceProperties::vendorID
    uint32_t deviceID;      // VkPhysicalDeviceProperties::deviceID
    uint32_t driverVersion; // VkPhysicalDeviceProperties::driverVersion
    uint32_t driverABI;     // sizeof(void*) to detect 32-bit vs 64-bit mismatch

    uint8_t uuid[VK_UUID_SIZE]; // VkPhysicalDeviceProperties::pipelineCacheUUID
};

class VulkanPipelineCache
{
public:
    static constexpr uint32_t kMagic = 0x474E5843u; // 'GNXC' in little-endian
    static const char* kCacheFileName;

    bool Initialize(VulkanContext& context);
    void SaveAndDestroy(VulkanContext& context);

    // Save pipeline cache data to disk (without destroying the VkPipelineCache object)
    // Can be called multiple times during the application lifetime
    bool Save(VulkanContext& context);

    VkPipelineCache GetHandle() const { return mPipelineCache; }
    bool IsValid() const { return mPipelineCache != VK_NULL_HANDLE; }

    // Static helpers (used by VulkanContext.cpp)
    static std::string GetCacheFilePath();
    static uint64_t ComputeHash(const void* data, size_t size);
    static bool ReadFileToBuffer(const std::string& path, std::vector<uint8_t>& buffer);
    static bool WriteBufferToFile(const std::string& path, const void* data, size_t size);

private:
    VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_PIPELINE_CACHE_INCLUDE_ADFGHJ */
