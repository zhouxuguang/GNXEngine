//
//  MTLDeviceExtension.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/3.
//

#ifndef GNX_ENGINE_MTL_DEVICE_EXTENSION_INCLUDE_DFGJJGJ
#define GNX_ENGINE_MTL_DEVICE_EXTENSION_INCLUDE_DFGJJGJ

#include "MTLRenderDefine.h"
#include "DeviceExtension.h"

NAMESPACE_RENDERCORE_BEGIN

class MTLDeviceExtension : public DeviceExtension
{
public:
    MTLDeviceExtension();
    
    ~MTLDeviceExtension();
    
    /** Whether or not the GPU supports NPOT (Non Power Of Two) textures.
     OpenGL ES 2.0 already supports NPOT (iOS).
     *
     * @return Is true if supports NPOT.
     * @since v0.99.2
     */
    virtual bool isSupportsNPOT() const;

    /** Whether or not PVR Texture Compressed is supported.
     *
     * @return Is true if supports PVR Texture Compressed.
     */
    virtual bool isSupportsPVRTC() const;

    /** Whether or not ETC Texture Compressed is supported.
     *
     *
     * @return Is true if supports ETC Texture Compressed.
     */
    virtual bool isSupportsETC() const;
    
    /** Whether or not ETC2 Texture Compressed is supported.
     *
     *
     * @return Is true if supports ETC2 Texture Compressed.
     */
    virtual bool isSupportsETC2() const;
    
    /** Whether or not ASTC Texture Compressed is supported.
     *
     *
     * @return Is true if supports ASTC Texture Compressed.
     */
    virtual bool isSupportsASTC() const;

    /** Whether or not S3TC Texture Compressed is supported.
     *
     * @return Is true if supports S3TC Texture Compressed.
     */
    virtual bool isSupportsS3TC() const;

    /** Whether or not ATITC Texture Compressed is supported.
     *
     * @return Is true if supports ATITC Texture Compressed.
     */
    virtual bool isSupportsATITC() const;

    /** Whether or not 3DC Texture Compressed is supported.
     *
     * @return Is true if supports 3DC Texture Compressed.
     */
    virtual bool isSupports3DC() const;

    /** Whether or not BGRA8888 textures are supported.
     *
     * @return Is true if supports BGRA8888 textures.
     * @since v0.99.2
     */
    virtual bool isSupportsBGRA8888() const;

    /** Whether or not glDiscardFramebufferEXT is supported.
     * @return Is true if supports glDiscardFramebufferEXT.
     * @since v0.99.2
     */
    virtual bool isSupportsDiscardFramebuffer() const;

    /** Whether or not OES_depth24 is supported.
     *
     * @return Is true if supports OES_depth24.
     * @since v2.0.0
     */
    virtual bool isSupportsOESDepth24() const;

    /** Whether or not OES_Packed_depth_stencil is supported.
     *
     * @return Is true if supports OES_Packed_depth_stencil.
     * @since v2.0.0
     */
    virtual bool isSupportsOESPackedDepthStencil() const;

    /** Whether or not glMapBuffer() is supported.
     *
     * On Desktop it returns `true`.
     * On Mobile it checks for the extension `GL_OES_mapbuffer`
     *
     * @return Whether or not `glMapBuffer()` is supported.
     * @since v3.13
     */
    virtual bool isSupportsMapBuffer() const;
    
    /** Whether or not GL_OES_standard_derivatives is supported.
     *
     * On Mobile it checks for the extension `GL_OES_standard_derivatives`
     *
     * @return Whether or not `GL_OES_standard_derivatives` is supported.
     * @since v1.1
     */
    virtual bool isSupportDerivative() const;
    
    virtual bool isSupportGeometryShader() const;
    
    virtual bool isSupportAnisotropic() const;
    
    virtual bool isSupportBinaryShader() const;
    
    //获得最大的纹理单元个数
    virtual int getMaxTextureUnits() const;
};

typedef std::shared_ptr<MTLDeviceExtension> MTLDeviceExtensionPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_MTL_DEVICE_EXTENSION_INCLUDE_DFGJJGJ */
