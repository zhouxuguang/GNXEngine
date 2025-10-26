/*
 * GLConfiguration.h
 *
 * 
 *      Author: zhouxuguang
 */

#ifndef GNX_ENGINE_GLCONFIGURATION_H__INCLUDE_H
#define GNX_ENGINE_GLCONFIGURATION_H__INCLUDE_H

#include "GLRenderDefine.h"
#include "DeviceExtension.h"

NAMESPACE_RENDERCORE_BEGIN


class GLDeviceExtension : public DeviceExtension
{
public:
    GLDeviceExtension();
    
	~GLDeviceExtension();

	void GatherGPUInfo();

    bool checkForGLExtension(const char *searchName) const;
    
    /** Whether or not the GPU supports NPOT (Non Power Of Two) textures.
     OpenGL ES 2.0 already supports NPOT (iOS).
     *
     * @return Is true if supports NPOT.
     * @since v0.99.2
     */
	bool isSupportsNPOT() const;

    /** Whether or not PVR Texture Compressed is supported.
     *
     * @return Is true if supports PVR Texture Compressed.
     */
	bool isSupportsPVRTC() const;

    /** Whether or not ETC Texture Compressed is supported.
     *
     *
     * @return Is true if supports ETC Texture Compressed.
     */
    bool isSupportsETC() const;
    
    /** Whether or not ETC2 Texture Compressed is supported.
     *
     *
     * @return Is true if supports ETC2 Texture Compressed.
     */
    bool isSupportsETC2() const
    {
        return true;
    }
    
    /** Whether or not ASTC Texture Compressed is supported.
     *
     *
     * @return Is true if supports ASTC Texture Compressed.
     */
    bool isSupportsASTC() const
    {
        return supportsASTC;
    }

    /** Whether or not S3TC Texture Compressed is supported.
     *
     * @return Is true if supports S3TC Texture Compressed.
     */
    bool isSupportsS3TC() const;

    /** Whether or not ATITC Texture Compressed is supported.
     *
     * @return Is true if supports ATITC Texture Compressed.
     */
    bool isSupportsATITC() const;

	/** Whether or not 3DC Texture Compressed is supported.
     *
     * @return Is true if supports 3DC Texture Compressed.
     */
	bool isSupports3DC() const;

    /** Whether or not BGRA8888 textures are supported.
     *
     * @return Is true if supports BGRA8888 textures.
     * @since v0.99.2
     */
	bool isSupportsBGRA8888() const;

    /** Whether or not glDiscardFramebufferEXT is supported.
     * @return Is true if supports glDiscardFramebufferEXT.
     * @since v0.99.2
     */
	bool isSupportsDiscardFramebuffer() const;

    /** Whether or not shareable VAOs are supported.
     *
     * @return Is true if supports shareable VAOs.
     * @since v2.0.0
     */
	bool isSupportsShareableVAO() const;

    /** Whether or not OES_depth24 is supported.
     *
     * @return Is true if supports OES_depth24.
     * @since v2.0.0
     */
    bool isSupportsOESDepth24() const;

    /** Whether or not OES_Packed_depth_stencil is supported.
     *
     * @return Is true if supports OES_Packed_depth_stencil.
     * @since v2.0.0
     */
    bool isSupportsOESPackedDepthStencil() const;

    /** Whether or not glMapBuffer() is supported.
     *
     * On Desktop it returns `true`.
     * On Mobile it checks for the extension `GL_OES_mapbuffer`
     *
     * @return Whether or not `glMapBuffer()` is supported.
     * @since v3.13
     */
    bool isSupportsMapBuffer() const;
    
    /** Whether or not GL_OES_standard_derivatives is supported.
     *
     * On Mobile it checks for the extension `GL_OES_standard_derivatives`
     *
     * @return Whether or not `GL_OES_standard_derivatives` is supported.
     * @since v1.1
     */
    bool isSupportDerivative() const;
    
    bool isSupportGeometryShader() const;
    
    bool isSupportAnisotropic() const;
    
    bool isSupportBinaryShader() const;
    
    bool isSupportVBO() const;
    
    bool isSupportedSeparateShader() const;
    
    //获得最大的纹理单元个数
    int getMaxTextureUnits() const;
    
    bool isSupportedCopyImage() const;

    bool isSupportedVertexHalfFloat() const;
    
private:
    GLint           maxTextureSize;
    GLint           maxTextureUnits;
    bool            supportsPVRTC;
    bool            supportsETC1;
    bool            supportsS3TC;
    bool            supportsATITC;
	bool            supports3DC;
    bool            supportsASTC;
    bool            supportsNPOT;
    bool            supportsBGRA8888;
    bool            supportsDiscardFramebuffer;
    bool            supportsShareableVAO;
    bool            supportsOESMapBuffer;
    bool            supportsOESDepth24;
    bool            supportsOESPackedDepthStencil;
    bool            supportsDerivative;
    bool            supportsGeometryShader;
    bool            supportsAnisotropic;       //是否支持各项异性滤波
    bool            supportsBinaryShader;
    bool            supportedVBO;
    bool            supportedSeparateShader;  //GL_EXT_separate_shader_objects
    bool            supportedCopyImage;
    bool            supportedVertexHalfFloat;
    
    char* glExtensions;     //gl扩展

};

typedef std::shared_ptr<GLDeviceExtension> GLDeviceExtensionPtr;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_GLCONFIGURATION_H__INCLUDE_H */
