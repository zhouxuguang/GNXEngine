//
//  VulkanPipelineCache.cpp
//  rendercore
//
//  Robust Vulkan Pipeline Cache with disk serialization.
//  Based on: https://zeux.io/2019/07/17/serializing-pipeline-cache/
//

#include "VulkanPipelineCache.h"
#include "Runtime/BaseLib/include/EnvironmentUtility.h"
#include <fstream>
#include <cstring>
#include <algorithm>

NAMESPACE_RENDERCORE_BEGIN

USING_NS_BASELIB

const char* VulkanPipelineCache::kCacheFileName = "pipeline_cache.bin";

std::string VulkanPipelineCache::GetCacheFilePath()
{
    return std::string("") + kCacheFileName;
}

uint64_t VulkanPipelineCache::ComputeHash(const void* data, size_t size)
{
    // Use xxhash via BaseLib's HashFunction if available, or fallback to FNV-1a
    return (uint64_t)HashFunction(data, size);
}

bool VulkanPipelineCache::ReadFileToBuffer(const std::string& path, std::vector<uint8_t>& buffer)
{
    std::string filePath = baselib::EnvironmentUtility::GetInstance().GetCurrentWorkingDir() + "\\" + path;
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0)
        return false;

    file.seekg(0, std::ios::beg);
    buffer.resize(static_cast<size_t>(fileSize));

    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
        return false;

    return true;
}

bool VulkanPipelineCache::WriteBufferToFile(const std::string& path, const void* data, size_t size)
{
    std::string filePath = baselib::EnvironmentUtility::GetInstance().GetCurrentWorkingDir() + "\\" + path;
    // Write to temp file first, then rename for atomicity (as recommended in the article)
    std::string tempPath = filePath + ".tmp";

    std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return false;

    file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    file.close();

    if (!file.good())
    {
        // Clean up temp file on failure
        std::remove(tempPath.c_str());
        return false;
    }

    // Atomic rename temp -> target
    int result = std::rename(tempPath.c_str(), path.c_str());

    return result == 0;
}

bool VulkanPipelineCache::Initialize(VulkanContext& context)
{
    std::vector<uint8_t> initialData;
    bool hasValidData = false;

    // Try to load cache from disk
    std::string cachePath = GetCacheFilePath();
    std::vector<uint8_t> fileData;

    if (ReadFileToBuffer(cachePath, fileData))
    {
        // Validate header
        if (fileData.size() >= sizeof(PipelineCachePrefixHeader))
        {
            const PipelineCachePrefixHeader* header =
                reinterpret_cast<const PipelineCachePrefixHeader*>(fileData.data());

            // Check magic number
            if (header->magic != kMagic)
            {
                LOG_INFO("PipelineCache: invalid magic number in cache file");
            }
            // Check vendor/device ID
            else if (header->vendorID != context.physicalDeviceProperties.vendorID ||
                     header->deviceID != context.physicalDeviceProperties.deviceID)
            {
                LOG_INFO("PipelineCache: device mismatch, ignoring cached data");
            }
            // Check driver version
            else if (header->driverVersion != context.physicalDeviceProperties.driverVersion)
            {
                LOG_INFO("PipelineCache: driver version changed, rebuilding cache");
            }
            // Check driver ABI (32-bit vs 64-bit)
            else if (header->driverABI != sizeof(void*))
            {
                LOG_INFO("PipelineCache: ABI mismatch (expected %u got %u), ignoring cache",
                         sizeof(void*), header->driverABI);
            }
            // Check pipeline cache UUID from physical device
            else if (memcmp(header->uuid,
                            context.physicalDeviceProperties.pipelineCacheUUID,
                            VK_UUID_SIZE) != 0)
            {
                LOG_INFO("PipelineCache: UUID mismatch, ignoring cached data");
            }
            // Validate data size
            else if (header->dataSize == 0 ||
                     header->dataSize + sizeof(PipelineCachePrefixHeader) > fileData.size())
            {
                LOG_INFO("PipelineCache: invalid data size in cache file");
            }
            // Validate hash for integrity
            else
            {
                const uint8_t* cacheData = fileData.data() + sizeof(PipelineCachePrefixHeader);
                uint64_t computedHash = ComputeHash(cacheData, header->dataSize);

                if (computedHash == header->dataHash)
                {
                    initialData.assign(cacheData, cacheData + header->dataSize);
                    hasValidData = true;
                    LOG_INFO("PipelineCache: loaded %u bytes of valid cached data",
                             header->dataSize);
                }
                else
                {
                    LOG_INFO("PipelineCache: hash mismatch, file may be corrupted");
                }
            }
        }
        else
        {
            LOG_INFO("PipelineCache: cache file too small");
        }
    }

    // Create VkPipelineCache with or without initial data
    VkPipelineCacheCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    createInfo.pInitialData = hasValidData ? initialData.data() : nullptr;
    createInfo.initialDataSize = hasValidData ? initialData.size() : 0;

    VkResult result = vkCreatePipelineCache(context.device, &createInfo,
                                            nullptr, &mPipelineCache);

    if (result != VK_SUCCESS)
    {
        // If creation with initial data fails, retry without data (driver bug workaround)
        if (hasValidData && mPipelineCache == VK_NULL_HANDLE)
        {
            LOG_INFO("PipelineCache: creation failed with cached data, "
                     "retrying without (possible driver bug)");

            createInfo.pInitialData = nullptr;
            createInfo.initialDataSize = 0;

            result = vkCreatePipelineCache(context.device, &createInfo,
                                           nullptr, &mPipelineCache);
        }

        if (result != VK_SUCCESS || mPipelineCache == VK_NULL_HANDLE)
        {
            LOG_INFO("PipelineCache: failed to create pipeline cache, will run without cache");
            mPipelineCache = VK_NULL_HANDLE;
            return false;
        }
    }

    LOG_INFO("PipelineCache: initialized successfully%s",
             hasValidData ? " (with cached data)" : " (empty)");
    return true;
}

bool VulkanPipelineCache::Save(VulkanContext& context)
{
    if (mPipelineCache == VK_NULL_HANDLE)
        return false;

    // Query cache data size
    size_t dataSize = 0;
    VkResult result = vkGetPipelineCacheData(context.device, mPipelineCache, &dataSize, nullptr);

    if (result != VK_SUCCESS || dataSize == 0)
    {
        LOG_INFO("PipelineCache: no data to save (size=%zu)", dataSize);
        return false;
    }

    // Retrieve actual cache data
    std::vector<uint8_t> cacheData(dataSize);
    result = vkGetPipelineCacheData(context.device, mPipelineCache,
                                    &dataSize, cacheData.data());

    if (result != VK_SUCCESS)
    {
        LOG_INFO("PipelineCache: failed to retrieve cache data");
        return false;
    }

    // Build prefix header with validation info
    PipelineCachePrefixHeader header = {};
    header.magic = kMagic;
    header.dataSize = static_cast<uint32_t>(dataSize);
    header.dataHash = ComputeHash(cacheData.data(), dataSize);
    header.vendorID = context.physicalDeviceProperties.vendorID;
    header.deviceID = context.physicalDeviceProperties.deviceID;
    header.driverVersion = context.physicalDeviceProperties.driverVersion;
    header.driverABI = sizeof(void*);
    memcpy(header.uuid, context.physicalDeviceProperties.pipelineCacheUUID, VK_UUID_SIZE);

    // Write header + cache data to file
    std::string cachePath = GetCacheFilePath();
    std::vector<uint8_t> outputData(sizeof(PipelineCachePrefixHeader) + dataSize);
    memcpy(outputData.data(), &header, sizeof(PipelineCachePrefixHeader));
    memcpy(outputData.data() + sizeof(PipelineCachePrefixHeader),
           cacheData.data(), dataSize);

    if (WriteBufferToFile(cachePath, outputData.data(), outputData.size()))
    {
        LOG_INFO("PipelineCache: saved %u bytes to disk", header.dataSize);
        return true;
    }
    else
    {
        LOG_INFO("PipelineCache: failed to save cache to disk");
        return false;
    }
}

void VulkanPipelineCache::SaveAndDestroy(VulkanContext& context)
{
    if (mPipelineCache == VK_NULL_HANDLE)
        return;

    Save(context);

    // Destroy the pipeline cache object
    vkDestroyPipelineCache(context.device, mPipelineCache, nullptr);
    mPipelineCache = VK_NULL_HANDLE;
}

NAMESPACE_RENDERCORE_END
