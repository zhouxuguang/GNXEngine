//
//  MTLPipelineCache.mm
//  GNXEngine
//
//  Metal Binary Archive (.metalar) based pipeline caching system.
//

#include "MTLPipelineCache.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "Runtime/BaseLib/include/EnvironmentUtility.h"

NAMESPACE_RENDERCORE_BEGIN

USING_NS_BASELIB

// ============================================================================
// Construction / Destruction
// ============================================================================

MTLPipelineCache::~MTLPipelineCache()
{
    SaveAndDestroy();
}

// ============================================================================
// Initialization
// ============================================================================

bool MTLPipelineCache::Initialize(id<MTLDevice> device)
{
#ifdef SUPPORTED_BINARY_ARCHIVE
    // Platform availability check: MTLBinaryArchive requires macOS 11+ / iOS 14+
    // (SDK header declares API_AVAILABLE(macos(11.0), ios(14.0)))
    if (!@available(macOS 11.0, iOS 14.0, *))
    {
        LOG_INFO("MTLPipelineCache: MTLBinaryArchive not available on this OS version, running without cache");
        return false; // Graceful degradation: cache is a no-op
    }

    mDevice = device;
    std::string cachePath = GetCacheFilePath();
    NSURL* cacheURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:cachePath.c_str()]];

    NSFileManager* fm = [NSFileManager defaultManager];
    BOOL isDir = NO;

    if ([fm fileExistsAtPath:cacheURL.path isDirectory:&isDir] && !isDir)
    {
        // Phase 2: Load existing archive from disk → use mode (archive as PSO creation hint)
        LOG_INFO("MTLPipelineCache: loading existing archive from %s", cachePath.c_str());

        NSError* loadError = nil;
        NSData* archivedData = [NSData dataWithContentsOfURL:cacheURL options:0 error:&loadError];

        if (archivedData && archivedData.length > 0 && !loadError)
        {
            MTLBinaryArchiveDescriptor* archiveDes = [MTLBinaryArchiveDescriptor new];
            archiveDes.url = cacheURL;

            // Correct signature: newBinaryArchiveWithDescriptor:error:
            NSError* createError = nil;
            mBinaryArchive = [device newBinaryArchiveWithDescriptor:archiveDes error:&createError];
            if (mBinaryArchive)
            {
                mIsCaptureMode = false;
                LOG_INFO("MTLPipelineCache: loaded archive in USE mode (%llu bytes)",
                         (unsigned long long)archivedData.length);
                return true;
            }
            else
            {
                LOG_WARN("MTLPipelineCache: failed to load archive, will start fresh in CAPTURE mode");
            }
        }
        else
        {
            if (loadError)
            {
                LOG_WARN("MTLPipelineCache: error reading archive file: %s",
                         loadError.localizedDescription.UTF8String);
            }
            else
            {
                LOG_WARN("MTLPipelineCache: archive file is empty or missing");
            }
        }

        // If loading failed, remove the corrupted file so we can recreate it cleanly
        [fm removeItemAtURL:cacheURL error:nil];
    }

    // Phase 1: No valid archive on disk → capture mode (collect PSOs into fresh archive)
    // Note: Capture happens IMPLICITLY when binaryArchives is set on the pipeline descriptor
    // before PSO creation. No manual addRenderPipelineState call needed.
    MTLBinaryArchiveDescriptor* archiveDes = [MTLBinaryArchiveDescriptor new];
    NSError* createError = nil;
    mBinaryArchive = [device newBinaryArchiveWithDescriptor:archiveDes error:&createError];

    if (mBinaryArchive)
    {
        mIsCaptureMode = true;
        LOG_INFO("MTLPipelineCache: created empty archive in CAPTURE mode");
        return true;
    }

    LOG_ERROR("MTLPipelineCache: failed to create binary archive");
#else
    (void)device;
    LOG_INFO("MTLPipelineCache: SDK does not support MTLBinaryArchive (requires macOS 12+/iOS 15+)");
#endif
    return false;
}

// ============================================================================
// Save / Destroy
// ============================================================================

std::string MTLPipelineCache::GetCacheFilePath() const
{
    std::string cwd = baselib::EnvironmentUtility::GetInstance().GetCurrentWorkingDir();
    // Normalize path separator: ensure forward slash for NSURL compatibility on macOS
#ifdef _WIN32
    // Windows: replace backslashes with forward slashes
    size_t pos = 0;
    while ((pos = cwd.find('\\', pos)) != std::string::npos)
        cwd[pos] = '/';
#endif
    if (!cwd.empty() && cwd.back() != '/')
        cwd += '/';
    return cwd + kDefaultCacheFileName;
}

bool MTLPipelineCache::Save()
{
#ifdef SUPPORTED_BINARY_ARCHIVE
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mBinaryArchive || !mDevice)
        return false;

    std::string cachePath = GetCacheFilePath();
    NSURL* cacheURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:cachePath.c_str()]];

    NSError* serializeError = nil;
    BOOL success = [mBinaryArchive serializeToURL:cacheURL error:&serializeError];

    if (!success)
    {
        LOG_ERROR("MTLPipelineCache: failed to serialize archive to disk: %s",
                  serializeError ? serializeError.localizedDescription.UTF8String : "unknown error");
        return false;
    }

    NSDictionary* attrs = [[NSFileManager defaultManager] attributesOfItemAtPath:cacheURL.path error:nil];
    unsigned long long fileSize = [attrs fileSize];
    LOG_INFO("MTLPipelineCache: saved archive to disk (%s, %llu bytes)",
             cachePath.c_str(), fileSize);
    return true;
#else
    return false;
#endif
}

void MTLPipelineCache::SaveAndDestroy()
{
#ifdef SUPPORTED_BINARY_ARCHIVE
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mBinaryArchive)
        return;

    // Serialize to disk (already holding the mutex)
    if (mDevice)
    {
        std::string cachePath = GetCacheFilePath();
        NSURL* cacheURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:cachePath.c_str()]];

        NSError* serializeError = nil;
        BOOL success = [mBinaryArchive serializeToURL:cacheURL error:&serializeError];

        if (success)
        {
            NSDictionary* attrs = [[NSFileManager defaultManager] attributesOfItemAtPath:cacheURL.path error:nil];
            unsigned long long fileSize = [attrs fileSize];
            LOG_INFO("MTLPipelineCache: saved archive at shutdown (%llu bytes)", fileSize);
        }
        else
        {
            LOG_ERROR("MTLPipelineCache: failed to save archive at shutdown: %s",
                      serializeError ? serializeError.localizedDescription.UTF8String : "unknown error");
        }
    }

    // ARC handles release automatically
    mBinaryArchive = nil;
    mDevice = nil;

    LOG_INFO("MTLPipelineCache: archive released");
#endif
}

// ============================================================================
// Accessors
// ============================================================================

id<MTLBinaryArchive> MTLPipelineCache::GetBinaryArchive() const
{
#ifdef SUPPORTED_BINARY_ARCHIVE
    std::lock_guard<std::mutex> lock(mMutex);
    return mBinaryArchive;
#else
    return nil;
#endif
}

bool MTLPipelineCache::HasValidArchive() const
{
#ifdef SUPPORTED_BINARY_ARCHIVE
    std::lock_guard<std::mutex> lock(mMutex);
    return mBinaryArchive != nil;
#else
    return false;
#endif
}

// ============================================================================
// Registration (Capture mode)
//
// Note: In Metal's Binary Archive design, PSO capture happens IMPLICITLY.
// When you set descriptor.binaryArchives = @[archive] BEFORE creating a PSO,
// the driver automatically adds successfully created PSOs to the archive.
// These methods are no-ops kept for API completeness.
// ============================================================================

void MTLPipelineCache::AddRenderPipelineState(id<MTLRenderPipelineState> pso)
{
    // Capture is implicit — no action needed here.
    (void)pso;
}

void MTLPipelineCache::AddComputePipelineState(id<MTLComputePipelineState> pso)
{
    // Capture is implicit — no action needed here.
    (void)pso;
}

NAMESPACE_RENDERCORE_END
