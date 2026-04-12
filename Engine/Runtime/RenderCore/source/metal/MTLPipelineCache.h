//
//  MTLPipelineCache.h
//  GNXEngine
//
//  Metal Binary Archive (.metalar) based pipeline caching system.
//  Supports macOS 12+ / iOS 15+ with graceful degradation.
//

#ifndef GNX_ENGINE_MTL_PIPELINE_CACHE_INCLUDE_H
#define GNX_ENGINE_MTL_PIPELINE_CACHE_INCLUDE_H

#include "MTLRenderDefine.h"
#import <Metal/Metal.h>
#include <mutex>

NAMESPACE_RENDERCORE_BEGIN

/**
 * Metal Pipeline Cache using MTLBinaryArchive (MTLPipelineLibrary).
 *
 * Workflow:
 *   Phase 1 (Capture): Create PSOs normally → add to archive → serialize to .metalar disk file.
 *   Phase 2 (Use):    Load .metalar → attach as hint when creating new PSOs → fast path hits.
 */
class MTLPipelineCache
{
public:
    static constexpr const char* kDefaultCacheFileName = "gnx_metal_pipeline_cache.metalar";

    MTLPipelineCache() = default;
    ~MTLPipelineCache();

    // Non-copyable, non-movable (owns Objective-C object)
    MTLPipelineCache(const MTLPipelineCache&) = delete;
    MTLPipelineCache& operator=(const MTLPipelineCache&) = delete;

    /**
     * Initialize the pipeline cache: try loading from disk, or create a fresh archive.
     * @param device The MTL device to create the archive on.
     * @return true if archive is usable (or gracefully degraded), false on error.
     */
    bool Initialize(id<MTLDevice> device);

    /**
     * Save captured PSOs to disk and release the archive.
     * Call this at application shutdown.
     */
    void SaveAndDestroy();

    /** Save current archive contents to disk without destroying. */
    bool Save();

    // ---- Accessors for pipeline creation ----

    /** Returns the underlying MTLBinaryArchive, or nil if unavailable. May be called from any thread. */
    id<MTLBinaryArchive> GetBinaryArchive() const;

    /** True if we are in capture mode (collecting PSOs). */
    bool IsCaptureMode() const { return mIsCaptureMode; }

    /** True if the archive is valid and can be used as a hint. */
    bool HasValidArchive() const;

    // ---- Registration (called after successful PSO creation) ----

    /** Register a newly created render pipeline state into the archive (capture mode). */
    void AddRenderPipelineState(id<MTLRenderPipelineState> pso);

    /** Register a newly created compute pipeline state into the archive (capture mode). */
    void AddComputePipelineState(id<MTLComputePipelineState> pso);

private:
    /** Build the absolute file path for the .metalar file. */
    std::string GetCacheFilePath() const;

    id<MTLBinaryArchive> mBinaryArchive = nil;
    id<MTLDevice> mDevice = nil;
    bool mIsCaptureMode = false;   // true = capturing PSOs; false = using archive as hint
    mutable std::mutex mMutex;
};

typedef std::shared_ptr<MTLPipelineCache> MTLPipelineCachePtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_PIPELINE_CACHE_INCLUDE_H */
