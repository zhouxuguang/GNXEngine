//
//  VKComputeBuffer.cpp
//  rendercore
//
//  Created by zhouxuguang on 2024/5/25.
//

#include "VKComputeBuffer.h"
#include "VulkanBufferUtil.h"

NAMESPACE_RENDERCORE_BEGIN

VKComputeBuffer::VKComputeBuffer(VulkanContextPtr context, size_t len, StorageMode mode) : mContext(context)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (mode == StorageModeShared)
    {
        memType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, mode, len,
                                      bufferUsage, memType, mBuffer, mAllocation, nullptr);
    
    mStorageMode = mode;
    mBufferLength = (uint32_t)len;
}

VKComputeBuffer::VKComputeBuffer(VulkanContextPtr context, const void* buffer, size_t size, StorageMode mode) : mContext(context)
{
    VkMemoryPropertyFlags memType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (mode == StorageModeShared)
    {
        memType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, mode, size,
                                      bufferUsage, memType, mBuffer, mAllocation, nullptr);
    
    if (mode == StorageModePrivate)
    {
        VkBuffer stageBuffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VulkanBufferUtil::CreateBufferVMA(mContext->vmaAllocator, StorageModeShared, size,
                        bufferUsage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memType, stageBuffer, allocation, nullptr);
        
        void *data = nullptr;
        vmaMapMemory(context->vmaAllocator, allocation, &data);
        memcpy(data, buffer, size);
        vmaUnmapMemory(context->vmaAllocator, allocation);
        
        VulkanBufferUtil::CopyBuffer(*mContext, mContext->graphicsQueue, mContext->GetCommandPool(), stageBuffer, mBuffer, size);
        vmaDestroyBuffer(mContext->vmaAllocator, stageBuffer, allocation);
    }
    else
    {
        void *data = nullptr;
        vmaMapMemory(context->vmaAllocator, mAllocation, &data);
        memcpy(data, buffer, size);
        vmaUnmapMemory(context->vmaAllocator, mAllocation);
    }
    
    mStorageMode = mode;
    mBufferLength = (uint32_t)size;
}

VKComputeBuffer::~VKComputeBuffer()
{
    vmaDestroyBuffer(mContext->vmaAllocator, mBuffer, mAllocation);
}

uint32_t VKComputeBuffer::GetBufferLength() const
{
    return mBufferLength;
}

void* VKComputeBuffer::MapBufferData() const
{
    if (mStorageMode == StorageModeShared)
    {
        void *data = nullptr;
        vmaMapMemory(mContext->vmaAllocator, mAllocation, &data);
        return data;
    }
    else
    {
        return NULL;
    }
}

void VKComputeBuffer::UnmapBufferData(void* bufferData) const
{
    vmaUnmapMemory(mContext->vmaAllocator, mAllocation);
}

bool VKComputeBuffer::IsValid() const
{
    return mBuffer != VK_NULL_HANDLE;
}

// RHI 抽象接口实现

ResourceState VKComputeBuffer::GetState() const
{
    return mResourceState;
}

void VKComputeBuffer::SetState(const ResourceState& state)
{
    mResourceState = state;
}

void VKComputeBuffer::PreReadBarrier(void* commandBuffer, ResourceAccess access, ResourcePipelineStage stage)
{
    if (!commandBuffer || !mBuffer)
    {
        return;
    }
    
    VkCommandBuffer cmdBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    
    // 计算旧的访问掩码和阶段
    uint32_t srcAccessMask = GetVulkanAccessMask(mResourceState.access);
    uint32_t srcStageMask = GetVulkanPipelineStageMask(mResourceState.stage);
    
    // 计算新的访问掩码和阶段
    uint32_t dstAccessMask = GetVulkanAccessMask(access);
    uint32_t dstStageMask = GetVulkanPipelineStageMask(stage);
    
    // 插入缓冲区屏障
    VulkanBufferUtil::InsertBufferMemoryBarrier(
        cmdBuffer,
        mBuffer,
        0, // offset
        mBufferLength, // size
        srcAccessMask,
        dstAccessMask,
        srcStageMask,
        dstStageMask
    );
    
    // 更新状态
    mResourceState.access = access;
    mResourceState.stage = stage;
    mResourceState.initialized = true;
}

void VKComputeBuffer::PreWriteBarrier(void* commandBuffer, ResourceAccess access, ResourcePipelineStage stage)
{
    if (!commandBuffer || !mBuffer)
    {
        return;
    }
    
    VkCommandBuffer cmdBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    
    // 计算旧的访问掩码和阶段
    uint32_t srcAccessMask = GetVulkanAccessMask(mResourceState.access);
    uint32_t srcStageMask = GetVulkanPipelineStageMask(mResourceState.stage);
    
    // 计算新的访问掩码和阶段
    uint32_t dstAccessMask = GetVulkanAccessMask(access);
    uint32_t dstStageMask = GetVulkanPipelineStageMask(stage);
    
    // 插入缓冲区屏障
    VulkanBufferUtil::InsertBufferMemoryBarrier(
        cmdBuffer,
        mBuffer,
        0, // offset
        mBufferLength, // size
        srcAccessMask,
        dstAccessMask,
        srcStageMask,
        dstStageMask
    );
    
    // 更新状态
    mResourceState.access = access;
    mResourceState.stage = stage;
    mResourceState.initialized = true;
}

// 转换辅助函数

uint32_t VKComputeBuffer::GetVulkanAccessMask(ResourceAccess access)
{
    uint32_t mask = 0;
    
    if ((access & ResourceAccess::VertexBuffer) != ResourceAccess::None)
    {
        mask |= (1 << 2); // VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    }
    if ((access & ResourceAccess::IndexBuffer) != ResourceAccess::None)
    {
        mask |= (1 << 3); // VK_ACCESS_INDEX_READ_BIT
    }
    if ((access & ResourceAccess::UniformBuffer) != ResourceAccess::None)
    {
        mask |= (1 << 4); // VK_ACCESS_UNIFORM_READ_BIT
    }
    if ((access & ResourceAccess::StorageBufferRead) != ResourceAccess::None)
    {
        mask |= (1 << 12); // VK_ACCESS_SHADER_READ_BIT
    }
    if ((access & ResourceAccess::StorageBufferWrite) != ResourceAccess::None)
    {
        mask |= (1 << 14); // VK_ACCESS_SHADER_WRITE_BIT
    }
    if ((access & ResourceAccess::IndirectBuffer) != ResourceAccess::None)
    {
        mask |= (1 << 1); // VK_ACCESS_INDIRECT_COMMAND_READ_BIT
    }
    if ((access & ResourceAccess::TransferSrc) != ResourceAccess::None)
    {
        mask |= (1 << 6); // VK_ACCESS_TRANSFER_READ_BIT
    }
    if ((access & ResourceAccess::TransferDst) != ResourceAccess::None)
    {
        mask |= (1 << 8); // VK_ACCESS_TRANSFER_WRITE_BIT
    }
    
    return mask;
}

uint32_t VKComputeBuffer::GetVulkanPipelineStageMask(ResourcePipelineStage stage)
{
    switch (stage)
    {
        case ResourcePipelineStage::TopOfPipe:
            return 0x00000001; // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        case ResourcePipelineStage::DrawIndirect:
            return 0x00000002; // VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
        case ResourcePipelineStage::VertexInput:
            return 0x00000004; // VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
        case ResourcePipelineStage::VertexShader:
            return 0x00000008; // VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        case ResourcePipelineStage::TessellationControlShader:
            return 0x00000010; // VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
        case ResourcePipelineStage::TessellationEvaluationShader:
            return 0x00000020; // VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
        case ResourcePipelineStage::GeometryShader:
            return 0x00000040; // VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
        case ResourcePipelineStage::FragmentShader:
            return 0x00000080; // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        case ResourcePipelineStage::EarlyFragmentTests:
            return 0x00000100; // VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        case ResourcePipelineStage::LateFragmentTests:
            return 0x00000200; // VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
        case ResourcePipelineStage::ColorAttachmentOutput:
            return 0x00000400; // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        case ResourcePipelineStage::ComputeShader:
            return 0x00000800; // VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        case ResourcePipelineStage::Transfer:
            return 0x00001000; // VK_PIPELINE_STAGE_TRANSFER_BIT
        case ResourcePipelineStage::BottomOfPipe:
            return 0x00002000; // VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        case ResourcePipelineStage::Host:
            return 0x00004000; // VK_PIPELINE_STAGE_HOST_BIT
        case ResourcePipelineStage::AllGraphics:
            return 0x000007FF; // VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT
        case ResourcePipelineStage::AllCommands:
            return 0x7FFFFFFF; // VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
        default:
            return 0x00002000; // VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    }
}

NAMESPACE_RENDERCORE_END
