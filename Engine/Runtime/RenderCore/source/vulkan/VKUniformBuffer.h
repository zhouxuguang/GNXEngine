//
//  VKUniformBuffer.h
//  rendercore
//
//  Created by zhouxuguang on 2024/5/26.
//

#ifndef GNX_ENGINE_VK_UNIFORM_BUFFER_INCLUDE_DHNJDHH
#define GNX_ENGINE_VK_UNIFORM_BUFFER_INCLUDE_DHNJDHH

#include "VulkanContext.h"
#include "UniformBuffer.h"

NAMESPACE_RENDERCORE_BEGIN

class VKUniformBuffer : public UniformBuffer
{
public:
    VKUniformBuffer(VulkanContextPtr context, uint32_t size, uint32_t slotCount);
    
    ~VKUniformBuffer();
    
    /**
     * @brief 只更新 CPU 端数据，真正的显存写入推迟到绑定时（PrepareForFrame）。
     *
     * 引擎允许最多 imageCount 帧同时在飞行：CPU 录制第 N 帧时，第 N-1、N-2 帧的
     * 命令可能还在 GPU 上执行。若 SetData 直接写进唯一的 GPU buffer，正在执行的
     * 命令会读到新数据 —— 同一帧内先录制的 draw 用旧相机矩阵、后录制的用新矩阵，
     * 表现为"移动相机时画面闪烁"。因此这里只写 CPU 影子副本。
     */
    void SetData(const void* data, uint32_t offset, uint32_t dataSize) override;
    
    /**
     * @brief 绑定前调用：确保 frameIndex 对应的帧槽位持有最新数据，返回该槽位在 buffer 中的字节偏移。
     *
     * 每个帧槽位只会被"间隔 slotCount 帧"的提交使用；只要 slotCount >= 交换链 image count，
     * 该槽位上一次被使用时的提交必然已经结束（CreateCommandBuffer 会等待该帧槽位的 fence），
     * 因此这里的写入不会与 GPU 的读取冲突。
     */
    VkDeviceSize PrepareForFrame(uint32_t frameIndex);
    
    VkBuffer GetBuffer() const
    {
        return mBuffer;
    }
    
    // 用于 push constant 路径：读取 CPU shadow copy
    const void* GetShadowData() const
    {
        return mShadowCopy.data();
    }
    
    uint32_t GetSize() const
    {
        return mBufferLength;
    }
    
private:
    void CreateSlots(uint32_t slotCount);
    void EnsureSlotCount(uint32_t slotCount);
    void UploadSlot(uint32_t slot);
    
    VulkanContextPtr mContext;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    StorageMode mStorageMode;
    uint32_t mBufferLength = 0;      // 逻辑大小（shader 看到的大小）
    uint32_t mSlotStride = 0;        // 单个帧槽位的对齐后大小
    uint32_t mSlotCount = 0;         // 帧槽位数量（>= 交换链 image count）
    VmaAllocation mAllocation = VK_NULL_HANDLE;
    
    uint8_t* mMappedData = nullptr;  // 常驻映射的 CPU 指针（HOST_COHERENT）
    
    // CPU 端数据副本：SetData 只改这里，绑定时再刷到对应帧槽位
    std::vector<uint8_t> mShadowCopy;
    uint64_t mDataVersion = 1;                 // ShadowCopy 的版本号
    std::vector<uint64_t> mSlotVersions;       // 每个帧槽位已上传的版本号
    
    // 扩容后遗留的旧 buffer，等待安全时机销毁
    std::vector<std::pair<VkBuffer, VmaAllocation>> mRetiredBuffers;
    
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_UNIFORM_BUFFER_INCLUDE_DHNJDHH */
