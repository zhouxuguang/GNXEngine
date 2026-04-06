//
//  VKDeviceExtension.h
//  GNXEngine
//
//  Mesh Shader Device Extension for Vulkan backend
//

#ifndef GNX_ENGINE_VK_DEVICE_EXTENSION_INCLUDE_H
#define GNX_ENGINE_VK_DEVICE_EXTENSION_INCLUDE_H

#include "VKRenderDefine.h"
#include "DeviceExtension.h"

NAMESPACE_RENDERCORE_BEGIN

struct VulkanContext;
typedef std::shared_ptr<struct VulkanContext> VulkanContextPtr;

class VKDeviceExtension : public DeviceExtension
{
public:
    VKDeviceExtension(VulkanContextPtr context);
    
    ~VKDeviceExtension();
    
    virtual bool isSupportsNPOT() const override { return true; }
    virtual bool isSupportsPVRTC() const override { return false; }
    virtual bool isSupportsETC() const override { return false; }
    virtual bool isSupportsETC2() const override { return true; }
    virtual bool isSupportsASTC() const override { return true; }
    virtual bool isSupportsS3TC() const override { return true; }
    virtual bool isSupportsATITC() const override { return false; }
    virtual bool isSupports3DC() const override { return false; }
    virtual bool isSupportsBGRA8888() const override { return true; }
    virtual bool isSupportsDiscardFramebuffer() const override { return true; }
    virtual bool isSupportsOESDepth24() const override { return true; }
    virtual bool isSupportsOESPackedDepthStencil() const override { return true; }
    virtual bool isSupportsMapBuffer() const override { return true; }
    virtual bool isSupportDerivative() const override { return true; }
    virtual bool isSupportGeometryShader() const override { return true; }
    virtual bool isSupportAnisotropic() const override { return true; }
    virtual bool isSupportBinaryShader() const override { return true; }
    virtual int getMaxTextureUnits() const override { return 32; }

    /// 是否支持 Mesh Shader（VK_EXT_mesh_shader 或 VK_NV_mesh_shader）
    virtual bool isSupportsMeshShader() const override;

    /// 是否支持 Task Shader（VK_EXT_mesh_shader 才支持 task shader，NV 不支持）
    virtual bool isSupportsTaskShader() const override;

private:
    VulkanContextPtr mContext = nullptr;
};

typedef std::shared_ptr<VKDeviceExtension> VKDeviceExtensionPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_VK_DEVICE_EXTENSION_INCLUDE_H */
