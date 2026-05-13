//
//  AftermathCrashTracker.cpp
//  GNXEngine
//
//  NVIDIA Nsight Aftermath SDK integration - GPU Crash Dump Tracker
//

#include "AftermathCrashTracker.h"

#ifdef ENABLE_NSIGHT_AFTERMATH

#include "Runtime/RenderCore/source/vulkan/VKRenderDefine.h"
#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"
#include "Runtime/BaseLib/include/LogService.h"

#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <windows.h>

NAMESPACE_RENDERCORE_BEGIN

// ============================================================================
// Aftermath API function pointer struct
// ============================================================================

struct AftermathFuncs
{
    PFN_GFSDK_Aftermath_EnableGpuCrashDumps  EnableGpuCrashDumps = nullptr;
    PFN_GFSDK_Aftermath_DisableGpuCrashDumps DisableGpuCrashDumps = nullptr;
    PFN_GFSDK_Aftermath_GetCrashDumpStatus   GetCrashDumpStatus = nullptr;
    PFN_GFSDK_Aftermath_GpuCrashDump_CreateDecoder   CreateDecoder = nullptr;
    PFN_GFSDK_Aftermath_GpuCrashDump_DestroyDecoder  DestroyDecoder = nullptr;
    PFN_GFSDK_Aftermath_GpuCrashDump_GetBaseInfo      GetBaseInfo = nullptr;
    PFN_GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize GetDescriptionSize = nullptr;
    PFN_GFSDK_Aftermath_GpuCrashDump_GetDescription  GetDescription = nullptr;
    PFN_GFSDK_Aftermath_GpuCrashDump_GenerateJSON    GenerateJSON = nullptr;
    PFN_GFSDK_Aftermath_GpuCrashDump_GetJSON         GetJSON = nullptr;
    PFN_GFSDK_Aftermath_GetShaderDebugInfoIdentifier GetShaderDebugInfoIdentifier = nullptr;
    PFN_GFSDK_Aftermath_GetShaderHashSpirv           GetShaderHashSpirv = nullptr;
    PFN_GFSDK_Aftermath_GetShaderDebugNameSpirv      GetShaderDebugNameSpirv = nullptr;
};

static AftermathFuncs* F(void* p) { return static_cast<AftermathFuncs*>(p); }

// ============================================================================
// Static callback forwarders (matching exact Aftermath SDK callback signatures)
// On x64, GFSDK_AFTERMATH_CALL expands to nothing.
// ============================================================================

static void GpuCrashDumpCb(const void* pGpuCrashDump, uint32_t gpuCrashDumpSize, void* pUserData)
{
    reinterpret_cast<AftermathCrashTracker*>(pUserData)->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
}

static void ShaderDebugInfoCb(const void* pShaderDebugInfo, uint32_t shaderDebugInfoSize, void* pUserData)
{
    reinterpret_cast<AftermathCrashTracker*>(pUserData)->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

static void CrashDumpDescriptionCb(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* pUserData)
{
    reinterpret_cast<AftermathCrashTracker*>(pUserData)->OnDescription(reinterpret_cast<void*>(addValue));
}

// PFN_GFSDK_Aftermath_ResolveMarkerCb: (const void*, uint32_t, void*, PFN_GFSDK_Aftermath_ResolveMarker)
static void ResolveMarkerCb(const void* pMarkerData, uint32_t markerDataSize, void* pUserData, PFN_GFSDK_Aftermath_ResolveMarker resolveMarker)
{
    reinterpret_cast<AftermathCrashTracker*>(pUserData)->OnResolveMarker(pMarkerData, markerDataSize, reinterpret_cast<void*>(resolveMarker));
}

static void ShaderDebugInfoLookupCb(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData)
{
    reinterpret_cast<AftermathCrashTracker*>(pUserData)->LookupShaderDebugInfo(pIdentifier, reinterpret_cast<void*>(setShaderDebugInfo));
}

static void ShaderLookupCb(const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
{
    reinterpret_cast<AftermathCrashTracker*>(pUserData)->LookupShader(pShaderHash, reinterpret_cast<void*>(setShaderBinary));
}

static void ShaderSourceDebugInfoLookupCb(const GFSDK_Aftermath_ShaderDebugName*, PFN_GFSDK_Aftermath_SetData, void*)
{
    // No source debug info available
}

// ============================================================================
// Singleton
// ============================================================================

AftermathCrashTracker& GetAftermathCrashTracker()
{
    static AftermathCrashTracker instance;
    return instance;
}

AftermathCrashTracker::AftermathCrashTracker() = default;
AftermathCrashTracker::~AftermathCrashTracker() { Shutdown(); }

// ============================================================================
// Initialize
// ============================================================================

bool AftermathCrashTracker::Initialize()
{
    if (mInitialized) return true;

    mDllHandle = LoadLibraryA("GFSDK_Aftermath_Lib.x64.dll");
    if (!mDllHandle)
    {
        LOG_WARN("AftermathCrashTracker: Cannot load GFSDK_Aftermath_Lib.x64.dll");
        return false;
    }

    auto* f = new AftermathFuncs();
    mFuncs = f;

    #define LOAD(api_name, member_name) \
        f->member_name = reinterpret_cast<PFN_##api_name>(GetProcAddress(static_cast<HMODULE>(mDllHandle), #api_name)); \
        if (!f->member_name) { LOG_ERROR("AftermathCrashTracker: Cannot find " #api_name); Shutdown(); return false; }

    LOAD(GFSDK_Aftermath_EnableGpuCrashDumps, EnableGpuCrashDumps);
    LOAD(GFSDK_Aftermath_DisableGpuCrashDumps, DisableGpuCrashDumps);
    LOAD(GFSDK_Aftermath_GetCrashDumpStatus, GetCrashDumpStatus);
    LOAD(GFSDK_Aftermath_GpuCrashDump_CreateDecoder, CreateDecoder);
    LOAD(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder, DestroyDecoder);
    LOAD(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo, GetBaseInfo);
    LOAD(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize, GetDescriptionSize);
    LOAD(GFSDK_Aftermath_GpuCrashDump_GetDescription, GetDescription);
    LOAD(GFSDK_Aftermath_GpuCrashDump_GenerateJSON, GenerateJSON);
    LOAD(GFSDK_Aftermath_GpuCrashDump_GetJSON, GetJSON);
    LOAD(GFSDK_Aftermath_GetShaderDebugInfoIdentifier, GetShaderDebugInfoIdentifier);
    LOAD(GFSDK_Aftermath_GetShaderHashSpirv, GetShaderHashSpirv);
    LOAD(GFSDK_Aftermath_GetShaderDebugNameSpirv, GetShaderDebugNameSpirv);
    #undef LOAD

    GFSDK_Aftermath_Result result = f->EnableGpuCrashDumps(
        GFSDK_Aftermath_Version_API,
        GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
        GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
        GpuCrashDumpCb, ShaderDebugInfoCb, CrashDumpDescriptionCb, ResolveMarkerCb, this);

    if (!GFSDK_Aftermath_SUCCEED(result))
    {
        LOG_ERROR("AftermathCrashTracker: EnableGpuCrashDumps failed (0x%08X)", result);
        Shutdown();
        return false;
    }

    mInitialized = true;
    LOG_INFO("AftermathCrashTracker: Initialized successfully");
    return true;
}

// ============================================================================
// Shutdown
// ============================================================================

void AftermathCrashTracker::Shutdown()
{
    if (!mInitialized && !mFuncs) return;

    if (mFuncs)
    {
        auto* f = F(mFuncs);
        if (f->DisableGpuCrashDumps) f->DisableGpuCrashDumps();
        delete f;
        mFuncs = nullptr;
    }

    if (mDllHandle)
    {
        FreeLibrary(static_cast<HMODULE>(mDllHandle));
        mDllHandle = nullptr;
    }

    mInitialized = false;
}

// ============================================================================
// RegisterShader
// ============================================================================

void AftermathCrashTracker::RegisterShader(const void* spirvCode, uint32_t spirvSize)
{
    if (!mInitialized || !spirvCode || spirvSize == 0) return;

    std::lock_guard<std::mutex> lock(mMutex);
    auto* f = F(mFuncs);

    GFSDK_Aftermath_SpirvCode spirv = { spirvCode, spirvSize };
    GFSDK_Aftermath_ShaderBinaryHash hash = {};
    if (GFSDK_Aftermath_SUCCEED(f->GetShaderHashSpirv(GFSDK_Aftermath_Version_API, &spirv, &hash)))
    {
        const uint8_t* data = static_cast<const uint8_t*>(spirvCode);
        mShaderDatabase[hash.hash] = std::vector<uint8_t>(data, data + spirvSize);

        // Write SPIR-V binary to aftermath directory
        std::string dumpDir = GetDumpDir();
        char hashStr[32];
        snprintf(hashStr, sizeof(hashStr), "%016llX", (unsigned long long)hash.hash);
        std::string filePath = dumpDir + "\\shader_" + std::string(hashStr) + ".spv";
        std::ofstream file(filePath, std::ios::binary);
        if (file.is_open())
        {
            file.write(reinterpret_cast<const char*>(spirvCode), spirvSize);
        }
    }
}

// ============================================================================
// SetCheckpoint
// ============================================================================

void AftermathCrashTracker::SetCheckpoint(VkCommandBuffer cmdBuffer, const char* marker)
{
    if (!mInitialized || !cmdBuffer || !marker) return;

    std::lock_guard<std::mutex> lock(mMutex);
    auto& stored = mMarkerMap[reinterpret_cast<const void*>(marker)];
    stored = marker;
    vkCmdSetCheckpointNV(cmdBuffer, stored.c_str());
}

// ============================================================================
// PollCrashDumpStatus
// ============================================================================

bool AftermathCrashTracker::PollCrashDumpStatus()
{
    if (!mInitialized || !mFuncs) return true;

    auto* f = F(mFuncs);
    GFSDK_Aftermath_CrashDump_Status status = GFSDK_Aftermath_CrashDump_Status_Unknown;
    f->GetCrashDumpStatus(&status);
    return (status == GFSDK_Aftermath_CrashDump_Status_Finished ||
            status == GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed);
}

bool AftermathCrashTracker::WaitForCrashDump(uint32_t timeoutMs)
{
    if (!mInitialized || !mFuncs) return false;

    auto* f = F(mFuncs);
    auto tStart = std::chrono::steady_clock::now();
    uint32_t elapsed = 0;

    while (elapsed < timeoutMs)
    {
        GFSDK_Aftermath_CrashDump_Status status = GFSDK_Aftermath_CrashDump_Status_Unknown;
        f->GetCrashDumpStatus(&status);

        if (status == GFSDK_Aftermath_CrashDump_Status_Finished)
            return true;
        if (status == GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed)
            return false;

        Sleep(50);
        auto tEnd = std::chrono::steady_clock::now();
        elapsed = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count());
    }

    return false;  // timeout
}

// ============================================================================
// GetDumpDir - get the "aftermath" directory under the exe path
// ============================================================================

std::string AftermathCrashTracker::GetDumpDir() const
{
    char exePath[260] = {};
    GetModuleFileNameA(nullptr, exePath, 260);
    std::string exeDir = exePath;
    auto lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        exeDir = exeDir.substr(0, lastSlash);

    std::string dumpDir = exeDir + "\\aftermath";
    CreateDirectoryA(dumpDir.c_str(), nullptr);
    return dumpDir;
}

// ============================================================================
// Callback handlers
// ============================================================================

void AftermathCrashTracker::OnCrashDump(const void* pGpuCrashDump, uint32_t gpuCrashDumpSize)
{
    std::lock_guard<std::mutex> lock(mMutex);
    LOG_ERROR("AftermathCrashTracker: GPU crash dump received (%u bytes)", gpuCrashDumpSize);
    WriteCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
}

void AftermathCrashTracker::OnShaderDebugInfo(const void* pShaderDebugInfo, uint32_t shaderDebugInfoSize)
{
    std::lock_guard<std::mutex> lock(mMutex);
    auto* f = F(mFuncs);

    GFSDK_Aftermath_ShaderDebugInfoIdentifier id = {};
    if (GFSDK_Aftermath_SUCCEED(f->GetShaderDebugInfoIdentifier(
        GFSDK_Aftermath_Version_API, pShaderDebugInfo, shaderDebugInfoSize, &id)))
    {
        char key[64];
        snprintf(key, sizeof(key), "%016llX_%016llX",
                 (unsigned long long)id.id[0], (unsigned long long)id.id[1]);
        const uint8_t* data = static_cast<const uint8_t*>(pShaderDebugInfo);
        mShaderDebugInfoMap[key] = std::vector<uint8_t>(data, data + shaderDebugInfoSize);

        // Write shader debug info to file
        std::string dumpDir = GetDumpDir();
        std::string filePath = dumpDir + "\\shader_debug_" + std::string(key) + ".bin";
        std::ofstream file(filePath, std::ios::binary);
        if (file.is_open())
        {
            file.write(reinterpret_cast<const char*>(pShaderDebugInfo), shaderDebugInfoSize);
            LOG_INFO("AftermathCrashTracker: Shader debug info written to %s", filePath.c_str());
        }
    }
}

void AftermathCrashTracker::OnDescription(void* addDescriptionFn)
{
    auto addDesc = reinterpret_cast<PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription>(addDescriptionFn);
    if (addDesc)
    {
        addDesc(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "GNXEngine");
        addDesc(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "0.1");
    }
}

void AftermathCrashTracker::OnResolveMarker(const void* pMarkerData, uint32_t markerDataSize, void* resolveMarkerFn)
{
    // Optionally call resolveMarker to update marker data before it's written to the crash dump.
    // For now, we just keep the existing marker data.
    auto resolveMarker = reinterpret_cast<PFN_GFSDK_Aftermath_ResolveMarker>(resolveMarkerFn);
    if (resolveMarker && pMarkerData && markerDataSize > 0)
    {
        resolveMarker(pMarkerData, markerDataSize);
    }
}

// ============================================================================
// Shader lookup handlers
// ============================================================================

void AftermathCrashTracker::LookupShaderDebugInfo(const void* pIdentifier, void* setShaderDebugInfoFn)
{
    auto* id = reinterpret_cast<const GFSDK_Aftermath_ShaderDebugInfoIdentifier*>(pIdentifier);
    auto setData = reinterpret_cast<PFN_GFSDK_Aftermath_SetData>(setShaderDebugInfoFn);

    std::lock_guard<std::mutex> lock(mMutex);

    char key[64];
    snprintf(key, sizeof(key), "%016llX_%016llX",
             (unsigned long long)id->id[0], (unsigned long long)id->id[1]);
    auto it = mShaderDebugInfoMap.find(key);
    if (it != mShaderDebugInfoMap.end())
    {
        setData(it->second.data(), static_cast<uint32_t>(it->second.size()));
    }
}

void AftermathCrashTracker::LookupShader(const void* pHash, void* setShaderBinaryFn)
{
    auto* hash = reinterpret_cast<const GFSDK_Aftermath_ShaderBinaryHash*>(pHash);
    auto setData = reinterpret_cast<PFN_GFSDK_Aftermath_SetData>(setShaderBinaryFn);

    std::lock_guard<std::mutex> lock(mMutex);

    auto it = mShaderDatabase.find(hash->hash);
    if (it != mShaderDatabase.end())
    {
        setData(it->second.data(), static_cast<uint32_t>(it->second.size()));
    }
}

// ============================================================================
// WriteCrashDumpToFile
// ============================================================================

void AftermathCrashTracker::WriteCrashDumpToFile(const void* pGpuCrashDump, uint32_t gpuCrashDumpSize)
{
    auto* f = F(mFuncs);

    std::string dumpDir = GetDumpDir();

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf;
    localtime_s(&tmBuf, &timeT);

    std::ostringstream oss;
    oss << dumpDir << "\\gpudump_" << std::put_time(&tmBuf, "%Y%m%d_%H%M%S") << ".nv-gpudmp";
    std::string dumpPath = oss.str();

    // Write raw crash dump
    {
        std::ofstream file(dumpPath, std::ios::binary);
        if (file.is_open())
        {
            file.write(static_cast<const char*>(pGpuCrashDump), gpuCrashDumpSize);
            LOG_INFO("AftermathCrashTracker: Dump written to %s", dumpPath.c_str());
        }
    }

    // Decode and generate JSON
    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
    GFSDK_Aftermath_Result result = f->CreateDecoder(
        GFSDK_Aftermath_Version_API, pGpuCrashDump, gpuCrashDumpSize, &decoder);

    if (!GFSDK_Aftermath_SUCCEED(result)) return;

    uint32_t jsonSize = 0;
    result = f->GenerateJSON(decoder, GFSDK_Aftermath_GpuCrashDumpDecoderFlags(0),
        GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
        ShaderDebugInfoLookupCb, ShaderLookupCb, ShaderSourceDebugInfoLookupCb,
        this, &jsonSize);

    if (GFSDK_Aftermath_SUCCEED(result) && jsonSize > 0)
    {
        std::vector<char> buf(jsonSize);
        if (GFSDK_Aftermath_SUCCEED(f->GetJSON(decoder, jsonSize, buf.data())))
        {
            std::string jsonPath = dumpPath;
            auto pos = jsonPath.rfind('.');
            if (pos != std::string::npos) jsonPath.replace(pos, std::string::npos, ".json");

            std::ofstream file(jsonPath, std::ios::binary);
            if (file.is_open())
                file.write(buf.data(), jsonSize);
        }
    }

    f->DestroyDecoder(decoder);
}

NAMESPACE_RENDERCORE_END

#endif // ENABLE_NSIGHT_AFTERMATH
