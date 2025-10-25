/*
 * GLConfiguration.cpp
 *
 *  
 *      Author: zhouxuguang
 */

#include "GLDeviceExtension.h"

NAMESPACE_RENDERCORE_BEGIN

bool IsETCSupported()
{
    int count = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count);
    if (count > 0)
    {
        GLint* formats = (GLint*)calloc(count, sizeof(GLint));
        
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);
        
        int index = 0;
        for (index = 0; index < count; index++)
        {
            if (formats[index] == GL_ETC1_RGB8_OES)
            {
                return true;
            }
        }
        free(formats);
    }
    return false;
}

GLDeviceExtension::GLDeviceExtension()
{
	// TODO Auto-generated constructor stub
    supportedVBO = true;
}

GLDeviceExtension::~GLDeviceExtension()
{
	// TODO Auto-generated destructor stub
}

void GLDeviceExtension::GatherGPUInfo()
{
    glExtensions = (char *)glGetString(GL_EXTENSIONS);
    
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);     //纹理最大尺寸
    
#if 1
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);    //最大纹理单元数
#else
    //glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxTextureUnits);    //最大纹理单元数
#endif
    
    supportsETC1 = checkForGLExtension("GL_OES_compressed_ETC1_RGB8_texture");  //OES_compressed_ETC1_RGB8_texture
    
    supportsS3TC = checkForGLExtension("GL_EXT_texture_compression_s3tc");
    
    supportsATITC = checkForGLExtension("GL_AMD_compressed_ATC_texture") || checkForGLExtension("GL_ATI_texture_compression_atitc");
    
    supportsPVRTC = checkForGLExtension("GL_IMG_texture_compression_pvrtc");

    supports3DC = checkForGLExtension("GL_AMD_compressed_3DC_texture");
    
    supportsASTC = checkForGLExtension("GL_OES_texture_compression_astc") ||
    checkForGLExtension("GL_KHR_texture_compression_astc_hdr") || checkForGLExtension("GL_KHR_texture_compression_astc_ldr");
    
    supportsNPOT = checkForGLExtension("GL_OES_texture_npot") ||
            checkForGLExtension("GL_IMG_texture_npot") || checkForGLExtension("GL_APPLE_texture_2D_limited_npot") || checkForGLExtension("GL_ARB_texture_non_power_of_two");
    
    supportsBGRA8888 = checkForGLExtension("GL_IMG_texture_format_BGRA888") || checkForGLExtension("GL_APPLE_texture_format_BGRA8888");
    
    supportsDiscardFramebuffer = checkForGLExtension("GL_EXT_discard_framebuffer");
    
    supportsShareableVAO = checkForGLExtension("vertex_array_object");
    
    supportsOESMapBuffer = checkForGLExtension("GL_OES_mapbuffer");
    
    supportsOESDepth24 = checkForGLExtension("GL_OES_depth24");
    
    supportsOESPackedDepthStencil = checkForGLExtension("GL_OES_packed_depth_stencil");
    
    supportsDerivative = checkForGLExtension("GL_OES_standard_derivatives");
    
    supportsGeometryShader = checkForGLExtension("GL_ARB_geometry_shader4") || checkForGLExtension("GL_EXT_geometry_shader4");
    
    supportsBinaryShader = checkForGLExtension("GL_OES_get_program_binary");
    
    // Check for Anisotropy support
    supportsAnisotropic = checkForGLExtension("GL_EXT_texture_filter_anisotropic");
    
    supportedSeparateShader = checkForGLExtension("GL_EXT_separate_shader_objects");
    
    if (supportsAnisotropic)
    {
        GLfloat maxAnisotropy = 0;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    }
    
    supportedCopyImage = checkForGLExtension("GL_EXT_copy_image");
    
    supportedVertexHalfFloat = checkForGLExtension("GL_OES_vertex_half_float");
    
    //查看是否支持渲染到浮点纹理
    bool islll =  checkForGLExtension("GL_EXT_color_buffer_float");
    
    bool isfffff = checkForGLExtension("GL_EXT_color_buffer_half_float");
    printf("");
}

bool GLDeviceExtension::checkForGLExtension(const char* searchName) const
{
    return  (glExtensions && searchName && strstr(glExtensions, searchName)) ? true : false;
}

bool GLDeviceExtension::isSupportsNPOT() const
{
    if (OpenGLESContext::isSupportGLES30())
    {
        return true;
    }
    return supportsNPOT;
}

bool GLDeviceExtension::isSupportsPVRTC() const
{
    return supportsPVRTC;
}

bool GLDeviceExtension::isSupportsETC() const
{
    if (OpenGLESContext::isSupportGLES30())
    {
        return true;
    }
#ifdef GL_ETC1_RGB8_OES
    return supportsETC1;
#else
    return false;
#endif
}

bool GLDeviceExtension::isSupportsS3TC() const
{
#ifdef GL_EXT_texture_compression_s3tc
    return supportsS3TC;
#else
    return false;
#endif
}

bool GLDeviceExtension::isSupports3DC() const
{
    return supports3DC;
}

bool GLDeviceExtension::isSupportsShareableVAO() const
{
    return supportsShareableVAO;
}

bool GLDeviceExtension::isSupportsATITC() const
{
    return supportsATITC;
}

bool GLDeviceExtension::isSupportsBGRA8888() const
{
    return supportsBGRA8888;
}

bool GLDeviceExtension::isSupportsDiscardFramebuffer() const
{
    return supportsDiscardFramebuffer;
}

bool GLDeviceExtension::isSupportsMapBuffer() const
{
    return supportsOESMapBuffer;
}

bool GLDeviceExtension::isSupportsOESDepth24() const
{
    return supportsOESDepth24;
}

bool GLDeviceExtension::isSupportsOESPackedDepthStencil() const
{
    return supportsOESPackedDepthStencil;
}

bool GLDeviceExtension::isSupportDerivative() const
{
    return supportsDerivative;
}

bool GLDeviceExtension::isSupportGeometryShader() const
{
    return supportsGeometryShader;
}

bool GLDeviceExtension::isSupportAnisotropic() const
{
    return supportsAnisotropic;
}

bool GLDeviceExtension::isSupportBinaryShader() const
{
    return supportsBinaryShader;
}

bool GLDeviceExtension::isSupportVBO() const
{
    return supportedVBO;
}

bool GLDeviceExtension::isSupportedSeparateShader() const
{
    return supportedSeparateShader;
}

//获得最大的纹理单元个数
int GLDeviceExtension::getMaxTextureUnits() const
{
    return maxTextureUnits;
}

bool GLDeviceExtension::isSupportedCopyImage() const
{
    if (OpenGLESContext::getVersionNumber() >= 32)
    {
        return true;
    }
    return supportedCopyImage;
}

bool GLDeviceExtension::isSupportedVertexHalfFloat() const
{
    if (OpenGLESContext::isSupportGLES30())
    {
        return true;
    }
    return supportedVertexHalfFloat;
}

NAMESPACE_RENDERCORE_END
