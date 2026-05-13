//
//  AftermathCrashTracker.h
//  GNXEngine
//
//  NVIDIA Nsight Aftermath SDK integration - GPU Crash Dump Tracker
//  Compile switch: ENABLE_NSIGHT_AFTERMATH
//

#ifndef GNX_ENGINE_AFTERMATH_CRASH_TRACKER_H
#define GNX_ENGINE_AFTERMATH_CRASH_TRACKER_H

#include "Runtime/RenderCore/source/vulkan/VKRenderDefine.h"

#ifdef ENABLE_NSIGHT_AFTERMATH

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

NAMESPACE_RENDERCORE_BEGIN

class AftermathCrashTracker
{
public:
    AftermathCrashTracker();
    ~AftermathCrashTracker();

    bool Initialize();
    void Shutdown();

    void RegisterShader(const void* spirvCode, uint32_t spirvSize);
    void SetCheckpoint(VkCommandBuffer cmdBuffer, const char* marker);
    bool PollCrashDumpStatus();
    bool WaitForCrashDump(uint32_t timeoutMs = 3000);
    bool IsInitialized() const { return mInitialized; }

    // Callback handlers (called from static callbacks)
    void OnCrashDump(const void* pGpuCrashDump, uint32_t gpuCrashDumpSize);
    void OnShaderDebugInfo(const void* pShaderDebugInfo, uint32_t shaderDebugInfoSize);
    void OnDescription(void* addDescriptionFn);
    void OnResolveMarker(const void* pMarkerData, uint32_t markerDataSize, void* resolveMarkerFn);

    // Shader debug info lookup (called from static lookup callbacks)
    void LookupShaderDebugInfo(const void* pIdentifier, void* setShaderDebugInfoFn);
    void LookupShader(const void* pHash, void* setShaderBinaryFn);

private:
    void WriteCrashDumpToFile(const void* pGpuCrashDump, uint32_t gpuCrashDumpSize);
    std::string GetDumpDir() const;

private:
    bool mInitialized = false;
    void* mDllHandle = nullptr;
    void* mFuncs = nullptr;  // opaque pointer to AftermathFuncs (defined in .cpp)

    std::mutex mMutex;
    std::unordered_map<uint64_t, std::vector<uint8_t>> mShaderDatabase;
    std::unordered_map<std::string, std::vector<uint8_t>> mShaderDebugInfoMap;
    std::unordered_map<const void*, std::string> mMarkerMap;
};

AftermathCrashTracker& GetAftermathCrashTracker();

NAMESPACE_RENDERCORE_END

#endif // ENABLE_NSIGHT_AFTERMATH

// Inline wrappers to avoid #ifdef at call sites
#ifndef ENABLE_NSIGHT_AFTERMATH

NAMESPACE_RENDERCORE_BEGIN

inline bool Aftermath_Initialize() { return false; }
inline void Aftermath_Shutdown() {}
inline void Aftermath_RegisterShader(const void*, uint32_t) {}
inline void Aftermath_SetCheckpoint(VkCommandBuffer, const char*) {}
inline bool Aftermath_PollCrashDumpStatus() { return true; }
inline bool Aftermath_WaitForCrashDump(uint32_t = 3000) { return true; }
inline bool Aftermath_IsInitialized() { return false; }

NAMESPACE_RENDERCORE_END

#else

NAMESPACE_RENDERCORE_BEGIN

inline bool Aftermath_Initialize() { return GetAftermathCrashTracker().Initialize(); }
inline void Aftermath_Shutdown() { return GetAftermathCrashTracker().Shutdown(); }
inline void Aftermath_RegisterShader(const void* code, uint32_t size) { GetAftermathCrashTracker().RegisterShader(code, size); }
inline void Aftermath_SetCheckpoint(VkCommandBuffer cmd, const char* marker) { GetAftermathCrashTracker().SetCheckpoint(cmd, marker); }
inline bool Aftermath_PollCrashDumpStatus() { return GetAftermathCrashTracker().PollCrashDumpStatus(); }
inline bool Aftermath_WaitForCrashDump(uint32_t timeoutMs = 3000) { return GetAftermathCrashTracker().WaitForCrashDump(timeoutMs); }
inline bool Aftermath_IsInitialized() { return GetAftermathCrashTracker().IsInitialized(); }

NAMESPACE_RENDERCORE_END

#endif // ENABLE_NSIGHT_AFTERMATH

#endif // GNX_ENGINE_AFTERMATH_CRASH_TRACKER_H
