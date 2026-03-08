//
//  RCBuffer.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/3/8.
//  Unified buffer interface for modern graphics APIs.
//

#ifndef GNX_ENGINE_RC_BUFFER_INCLUDE_H
#define GNX_ENGINE_RC_BUFFER_INCLUDE_H

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

/**
 * @brief Buffer usage flags - describes how a buffer can be used
 * 
 * Inspired by Vulkan's VkBufferUsageFlags and Metal's unified buffer approach.
 * Multiple flags can be combined to create a buffer that serves multiple purposes.
 * For example: RCBufferUsage::Vertex | RCBufferUsage::Storage for a buffer
 * used both as vertex input and SSBO in compute shaders.
 */
enum class RCBufferUsage : uint32_t
{
    Unknown         = 0x0000,
    VertexBuffer    = 0x0001,   // Used as vertex buffer in graphics pipeline
    IndexBuffer     = 0x0002,   // Used as index buffer in graphics pipeline
    UniformBuffer   = 0x0004,   // Used as uniform buffer (UBO)
    StorageBuffer   = 0x0008,   // Used as storage buffer (SSBO) for compute/fragment shaders
    IndirectBuffer  = 0x0010,   // Used for indirect draw/dispatch commands
    TransferSrc     = 0x0020,   // Used as source for transfer operations
    TransferDst     = 0x0040,   // Used as destination for transfer operations
};

// RCBufferUsage bitwise operators
inline constexpr RCBufferUsage operator|(RCBufferUsage lhs, RCBufferUsage rhs) noexcept
{
    return static_cast<RCBufferUsage>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline constexpr RCBufferUsage operator&(RCBufferUsage lhs, RCBufferUsage rhs) noexcept
{
    return static_cast<RCBufferUsage>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline constexpr RCBufferUsage operator^(RCBufferUsage lhs, RCBufferUsage rhs) noexcept
{
    return static_cast<RCBufferUsage>(
        static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs));
}

inline constexpr RCBufferUsage operator~(RCBufferUsage value) noexcept
{
    return static_cast<RCBufferUsage>(~static_cast<uint32_t>(value));
}

inline constexpr RCBufferUsage& operator|=(RCBufferUsage& lhs, RCBufferUsage rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

inline constexpr RCBufferUsage& operator&=(RCBufferUsage& lhs, RCBufferUsage rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

// Helper functions for RCBufferUsage
inline constexpr bool HasUsage(RCBufferUsage value, RCBufferUsage flag) noexcept
{
    return (value & flag) == flag;
}

inline constexpr bool HasAnyUsage(RCBufferUsage value, RCBufferUsage flags) noexcept
{
    return (value & flags) != RCBufferUsage::Unknown;
}

/**
 * @brief Buffer descriptor for creating RCBuffer
 */
struct RCBufferDesc
{
    uint32_t        size = 0;           // Buffer size in bytes
    RCBufferUsage   usage = RCBufferUsage::Unknown;  // Buffer usage flags
    StorageMode     storageMode = StorageModePrivate; // Memory storage mode
    
    RCBufferDesc() = default;
    
    RCBufferDesc(uint32_t _size, RCBufferUsage _usage, StorageMode _mode = StorageModePrivate)
        : size(_size), usage(_usage), storageMode(_mode) {}
};

/**
 * @brief Unified buffer interface for modern graphics APIs
 * 
 * RCBuffer replaces the separate VertexBuffer, ComputeBuffer classes.
 * This unified approach matches modern graphics APIs like Vulkan and Metal,
 * where the same buffer can serve multiple purposes.
 * 
 * Usage example:
 * @code
 * // Create a buffer for vertex data
 * auto vertexBuffer = device->CreateBuffer(RCBufferDesc(1024, RCBufferUsage::VertexBuffer));
 * 
 * // Create a buffer used both as SSBO and vertex buffer (common in GPU-driven rendering)
 * auto combinedBuffer = device->CreateBuffer(RCBufferDesc(
 *     4096, 
 *     RCBufferUsage::StorageBuffer | RCBufferUsage::VertexBuffer
 * ));
 * @endcode
 */
class RCBuffer
{
public:
    RCBuffer() = default;
    virtual ~RCBuffer() = default;
    
    /**
     * @brief Get buffer size in bytes
     */
    virtual uint32_t GetSize() const = 0;
    
    /**
     * @brief Get buffer usage flags
     */
    virtual RCBufferUsage GetUsage() const = 0;
    
    /**
     * @brief Map buffer memory for CPU access
     * @return Pointer to buffer data, nullptr if not mappable
     */
    virtual void* Map() const = 0;
    
    /**
     * @brief Unmap previously mapped buffer
     */
    virtual void Unmap() const = 0;
    
    /**
     * @brief Check if buffer is valid
     */
    virtual bool IsValid() const = 0;
    
    /**
     * @brief Set buffer name for debugging
     */
    virtual void SetName(const char* name) = 0;
    
    // Convenience methods for common usage patterns
    
    /**
     * @brief Check if this buffer can be used as vertex buffer
     */
    bool IsVertexBuffer() const { return HasUsage(GetUsage(), RCBufferUsage::VertexBuffer); }
    
    /**
     * @brief Check if this buffer can be used as storage buffer (SSBO)
     */
    bool IsStorageBuffer() const { return HasUsage(GetUsage(), RCBufferUsage::StorageBuffer); }
    
    /**
     * @brief Check if this buffer can be used as index buffer
     */
    bool IsIndexBuffer() const { return HasUsage(GetUsage(), RCBufferUsage::IndexBuffer); }
    
    /**
     * @brief Check if this buffer can be used as uniform buffer
     */
    bool IsUniformBuffer() const { return HasUsage(GetUsage(), RCBufferUsage::UniformBuffer); }
    
    /**
     * @brief Check if this buffer can be used as indirect buffer
     */
    bool IsIndirectBuffer() const { return HasUsage(GetUsage(), RCBufferUsage::IndirectBuffer); }
};

typedef std::shared_ptr<RCBuffer> RCBufferPtr;
typedef std::weak_ptr<RCBuffer> RCBufferWeakPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_RC_BUFFER_INCLUDE_H */
