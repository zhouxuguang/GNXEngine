//
//  MTLDeviceExtension.mm
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/3.
//

#include "MTLDeviceExtension.h"

NAMESPACE_RENDERCORE_BEGIN

MTLDeviceExtension::MTLDeviceExtension()
{
}

MTLDeviceExtension::~MTLDeviceExtension()
{
    //
}

/** Whether or not the GPU supports NPOT (Non Power Of Two) textures.
 OpenGL ES 2.0 already supports NPOT (iOS).
 *
 * @return Is true if supports NPOT.
 * @since v0.99.2
 */
bool MTLDeviceExtension::isSupportsNPOT() const
{
    return false;
}

/** Whether or not PVR Texture Compressed is supported.
 *
 * @return Is true if supports PVR Texture Compressed.
 */
bool MTLDeviceExtension::isSupportsPVRTC() const
{
    return true;
}

/** Whether or not ETC Texture Compressed is supported.
 *
 *
 * @return Is true if supports ETC Texture Compressed.
 */
bool MTLDeviceExtension::isSupportsETC() const
{
    return false;
}

/** Whether or not ETC2 Texture Compressed is supported.
 *
 *
 * @return Is true if supports ETC2 Texture Compressed.
 */
bool MTLDeviceExtension::isSupportsETC2() const
{
    return true;
}

/** Whether or not ASTC Texture Compressed is supported.
 *
 *
 * @return Is true if supports ASTC Texture Compressed.
 */
bool MTLDeviceExtension::isSupportsASTC() const
{
    return true;
}

/** Whether or not S3TC Texture Compressed is supported.
 *
 * @return Is true if supports S3TC Texture Compressed.
 */
bool MTLDeviceExtension::isSupportsS3TC() const
{
    return true;
}

/** Whether or not ATITC Texture Compressed is supported.
 *
 * @return Is true if supports ATITC Texture Compressed.
 */
bool MTLDeviceExtension::isSupportsATITC() const
{
    return false;
}

/** Whether or not 3DC Texture Compressed is supported.
 *
 * @return Is true if supports 3DC Texture Compressed.
 */
bool MTLDeviceExtension::isSupports3DC() const
{
    return false;
}

/** Whether or not BGRA8888 textures are supported.
 *
 * @return Is true if supports BGRA8888 textures.
 * @since v0.99.2
 */
bool MTLDeviceExtension::isSupportsBGRA8888() const
{
    return true;
}

/** Whether or not glDiscardFramebufferEXT is supported.
 * @return Is true if supports glDiscardFramebufferEXT.
 * @since v0.99.2
 */
bool MTLDeviceExtension::isSupportsDiscardFramebuffer() const
{
    return true;
}

/** Whether or not OES_depth24 is supported.
 *
 * @return Is true if supports OES_depth24.
 * @since v2.0.0
 */
bool MTLDeviceExtension::isSupportsOESDepth24() const
{
    return true;
}

/** Whether or not OES_Packed_depth_stencil is supported.
 *
 * @return Is true if supports OES_Packed_depth_stencil.
 * @since v2.0.0
 */
bool MTLDeviceExtension::isSupportsOESPackedDepthStencil() const
{
    return true;
}

/** Whether or not glMapBuffer() is supported.
 *
 * On Desktop it returns `true`.
 * On Mobile it checks for the extension `GL_OES_mapbuffer`
 *
 * @return Whether or not `glMapBuffer()` is supported.
 * @since v3.13
 */
bool MTLDeviceExtension::isSupportsMapBuffer() const
{
    return true;
}

/** Whether or not GL_OES_standard_derivatives is supported.
 *
 * On Mobile it checks for the extension `GL_OES_standard_derivatives`
 *
 * @return Whether or not `GL_OES_standard_derivatives` is supported.
 * @since v1.1
 */
bool MTLDeviceExtension::isSupportDerivative() const
{
    return true;
}

bool MTLDeviceExtension::isSupportGeometryShader() const
{
    return true;
}

bool MTLDeviceExtension::isSupportAnisotropic() const
{
    return true;
}

bool MTLDeviceExtension::isSupportBinaryShader() const
{
    return true;
}

//获得最大的纹理单元个数
int MTLDeviceExtension::getMaxTextureUnits() const
{
    return 32;
}

NAMESPACE_RENDERCORE_END
