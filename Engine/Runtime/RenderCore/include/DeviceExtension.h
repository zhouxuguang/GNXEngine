//
//  DeviceExtension.h
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/3.
//

#ifndef GNX_ENGINE_DEVICE_ENTENSION_INCLUDE_HPP_IDFG
#define GNX_ENGINE_DEVICE_ENTENSION_INCLUDE_HPP_IDFG

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

class DeviceExtension
{
public:
    DeviceExtension();
    
    virtual ~DeviceExtension();
    
    /** Whether or not the GPU supports NPOT (Non Power Of Two) textures.
     OpenGL ES 2.0 already supports NPOT (iOS).
     *
     * @return Is true if supports NPOT.
     * @since v0.99.2
     */
    virtual bool isSupportsNPOT() const = 0;

    /** Whether or not PVR Texture Compressed is supported.
     *
     * @return Is true if supports PVR Texture Compressed.
     */
    virtual bool isSupportsPVRTC() const = 0;

    /** Whether or not ETC Texture Compressed is supported.
     *
     *
     * @return Is true if supports ETC Texture Compressed.
     */
    virtual bool isSupportsETC() const = 0;
    
    /** Whether or not ETC2 Texture Compressed is supported.
     *
     *
     * @return Is true if supports ETC2 Texture Compressed.
     */
    virtual bool isSupportsETC2() const = 0;
    
    /** Whether or not ASTC Texture Compressed is supported.
     *
     *
     * @return Is true if supports ASTC Texture Compressed.
     */
    virtual bool isSupportsASTC() const = 0;

    /** Whether or not S3TC Texture Compressed is supported.
     *
     * @return Is true if supports S3TC Texture Compressed.
     */
    virtual bool isSupportsS3TC() const = 0;

    /** Whether or not ATITC Texture Compressed is supported.
     *
     * @return Is true if supports ATITC Texture Compressed.
     */
    virtual bool isSupportsATITC() const = 0;

    /** Whether or not 3DC Texture Compressed is supported.
     *
     * @return Is true if supports 3DC Texture Compressed.
     */
    virtual bool isSupports3DC() const = 0;

    /** Whether or not BGRA8888 textures are supported.
     *
     * @return Is true if supports BGRA8888 textures.
     * @since v0.99.2
     */
    virtual bool isSupportsBGRA8888() const = 0;

    /** Whether or not glDiscardFramebufferEXT is supported.
     * @return Is true if supports glDiscardFramebufferEXT.
     * @since v0.99.2
     */
    virtual bool isSupportsDiscardFramebuffer() const = 0;

    /** Whether or not OES_depth24 is supported.
     *
     * @return Is true if supports OES_depth24.
     * @since v2.0.0
     */
    virtual bool isSupportsOESDepth24() const = 0;

    /** Whether or not OES_Packed_depth_stencil is supported.
     *
     * @return Is true if supports OES_Packed_depth_stencil.
     * @since v2.0.0
     */
    virtual bool isSupportsOESPackedDepthStencil() const = 0;

    /** Whether or not glMapBuffer() is supported.
     *
     * On Desktop it returns `true`.
     * On Mobile it checks for the extension `GL_OES_mapbuffer`
     *
     * @return Whether or not `glMapBuffer()` is supported.
     * @since v3.13
     */
    virtual bool isSupportsMapBuffer() const = 0;
    
    /** Whether or not GL_OES_standard_derivatives is supported.
     *
     * On Mobile it checks for the extension `GL_OES_standard_derivatives`
     *
     * @return Whether or not `GL_OES_standard_derivatives` is supported.
     * @since v1.1
     */
    virtual bool isSupportDerivative() const = 0;
    
    virtual bool isSupportGeometryShader() const = 0;
    
    virtual bool isSupportAnisotropic() const = 0;
    
    virtual bool isSupportBinaryShader() const = 0;
    
    //获得最大的纹理单元个数
    virtual int getMaxTextureUnits() const = 0;
};

typedef std::shared_ptr<DeviceExtension> DeviceExtensionPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DEVICE_ENTENSION_INCLUDE_HPP_IDFG */
