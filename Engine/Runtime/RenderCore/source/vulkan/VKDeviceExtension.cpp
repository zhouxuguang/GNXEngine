//
//  VKDeviceExtension.cpp
//  GNXEngine
//

#include "VKDeviceExtension.h"
#include "VulkanContext.h"

NAMESPACE_RENDERCORE_BEGIN

VKDeviceExtension::VKDeviceExtension(VulkanContextPtr context)
{
    mContext = context;
}

VKDeviceExtension::~VKDeviceExtension()
{
    //
}

bool VKDeviceExtension::isSupportsMeshShader() const
{
    if (!mContext)
    {
        return false;
    }
    return mContext->vulkanExtension.enableMeshShaderEXT ||
           mContext->vulkanExtension.enableMeshShaderNV;
}

bool VKDeviceExtension::isSupportsTaskShader() const
{
    if (!mContext)
    {
        return false;
    }
    // 只有 VK_EXT_mesh_shader 支持 Task Shader
    // VK_NV_mesh_shader 不支持 Task Shader（只有 mesh + amplification 的简化版）
    return mContext->vulkanExtension.enableMeshShaderEXT;
}

NAMESPACE_RENDERCORE_END
