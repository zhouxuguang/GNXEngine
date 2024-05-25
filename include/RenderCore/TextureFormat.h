#ifndef RENDER_CORE_TEXTURE_FORMAT_INCLKUDE_H
#define RENDER_CORE_TEXTURE_FORMAT_INCLKUDE_H

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

//纹理格式枚举
typedef uint32_t TextureFormat;
enum
{
    kTexFormatInvalid = 0,
    kTexFormatAlpha8 = 1,
    kTexFormatLuma = 2,
    kTexFormatARGB4444 = 3,
    kTexFormatRGB24 = 4,
    kTexFormatRGBA32 = 5,
    kTexFormatARGB32 = 6,
    kTexFormatARGBFloat = 8, // only for internal use at runtime
    kTexFormatRGB565 = 9,
    kTexFormatRGBA5551 = 10,
    kTexFormatBGR24 = 11,
    kTexFormatRGBA4444 = 12,
    
    //srgb
    kTexFormatSRGB8 = 13,
    kTexFormatSRGB8_ALPHA8 = 14,
    
    
    // This one is for internal use; storage is 16 bits/pixel; samples
    // as Alpha (OpenGL) or RGB (D3D9). Can be reduced to 8 bit alpha/luminance on lower hardware.
    // Why it's not Luminance on GL: for some reason alpha seems to be faster.
    kTexFormatAlphaLum16 = 20,
    kTexFormatDXT1 = 21,
    kTexFormatDXT3 = 22,
    kTexFormatDXT5 = 23,
    

    kTexFormatPCCount = 24,

    // iPhone
    kTexFormatPVRTC_RGB2 = 30,
    kTexFormatPVRTC_RGBA2 = 31,

    kTexFormatPVRTC_RGB4 = 32,
    kTexFormatPVRTC_RGBA4 = 33,

    kTexFormatETC_RGB4 = 34,

    kTexFormatATC_RGB4 = 35,
    kTexFormatATC_RGBA8 = 36,

    // Pixels returned by iPhone camera
    kTexFormatBGRA32 = 37,

    // EAC and ETC2 compressed formats, mandated by OpenGL ES 3.0
    kTexFormatEAC_R = 41,
    kTexFormatEAC_R_SIGNED = 42,
    kTexFormatEAC_RG = 43,
    kTexFormatEAC_RG_SIGNED = 44,
    kTexFormatETC2_RGB = 45,
    kTexFormatETC2_SRGB = 46,
    kTexFormatETC2_RGBA1 = 47,
    kTexFormatETC2_SRGBA1 = 48,
    kTexFormatETC2_RGBA8 = 49,
    kTexFormatETC2_SRGBA8 = 50,
    
    kTexFormatETC1_RGB = 51,

    // ASTC. The RGB and RGBA formats are internally identical, we just need to carry the has-alpha information somehow
    kTexFormatASTC_RGB_4x4 = 54,
    kTexFormatASTC_RGB_5x5 = 55,
    kTexFormatASTC_RGB_6x6 = 56,
    kTexFormatASTC_RGB_8x8 = 57,
    kTexFormatASTC_RGB_10x10 = 58,
    kTexFormatASTC_RGB_12x12 = 59,

    kTexFormatASTC_RGBA_4x4 = 60,
    kTexFormatASTC_RGBA_5x5 = 61,
    kTexFormatASTC_RGBA_6x6 = 62,
    kTexFormatASTC_RGBA_8x8 = 63,
    kTexFormatASTC_RGBA_10x10 = 64,
    kTexFormatASTC_RGBA_12x12 = 65,
    
    //深度模板格式
    kTexFormatDepth16 = 70,            // 16 bit depth buffer
    kTexFormatDepth24 = 71,            // 24 bit depth buffer
    kTexFormatDepth32 = 72,            // 32 bit depth buffer
    kTexFormatDepth32Float = 73,       // 32 bit float depth buffer
    
    kTexFormatDepth24Stencil8 = 74,
    kTexFormatDepth32FloatStencil8 = 75,
    
    
    //浮点格式纹理
    kTexFormatRGBA16Float = 80,
    kTexFormatRGBA32Float = 81,

    kTexFormatTotalCount    = 100 // keep this last!
};

/* Important note about endianess.
   Endianess needs to be swapped for the following formats:
   kTexFormatARGBFloat, kTexFormatRGB565, kTexFormatARGB4444, (assuming for this too: kTexFormatRGBA4444)
*/

uint32_t GetBytesFromTextureFormat(TextureFormat inFormat);
uint32_t GetMaxBytesPerPixel(TextureFormat inFormat );
int GetRowBytesFromWidthAndFormat(int width, TextureFormat format);
bool IsValidTextureFormat(TextureFormat format);


inline bool IsCompressedDXTTextureFormat(TextureFormat format)
{
	return format >= kTexFormatDXT1 && format <= kTexFormatDXT5;
}

inline bool IsCompressedPVRTCTextureFormat(TextureFormat format)
{
	return format >= kTexFormatPVRTC_RGB2 && format <= kTexFormatPVRTC_RGBA4;
}

inline bool IsCompressedETCTextureFormat(TextureFormat format)
{
	return format == kTexFormatETC_RGB4;
}

inline bool IsCompressedEACTextureFormat(TextureFormat format)
{
	return format >= kTexFormatEAC_R && format <= kTexFormatEAC_RG_SIGNED;
}

inline bool IsCompressedETC2TextureFormat(TextureFormat format)
{
	return format >= kTexFormatETC2_RGB && format <= kTexFormatETC2_SRGBA8;
}

inline bool IsCompressedATCTextureFormat(TextureFormat format)
{
	return format == kTexFormatATC_RGB4 || format == kTexFormatATC_RGBA8;
}

inline bool Is16BitTextureFormat(TextureFormat format)
{
	return format == kTexFormatARGB4444 || format == kTexFormatRGBA4444 || format == kTexFormatRGB565;
}

inline bool IsCompressedASTCTextureFormat(TextureFormat format)
{
	return format >= kTexFormatASTC_RGB_4x4 && format <= kTexFormatASTC_RGBA_12x12;
}

inline bool IsAnyCompressedTextureFormat(TextureFormat format)
{
	return     IsCompressedDXTTextureFormat(format) || IsCompressedPVRTCTextureFormat(format)
			|| IsCompressedETCTextureFormat(format) || IsCompressedATCTextureFormat(format)
            || IsCompressedEACTextureFormat(format)
			|| IsCompressedETC2TextureFormat(format) || IsCompressedASTCTextureFormat(format) || kTexFormatETC1_RGB == format;
}

bool IsAlphaOnlyTextureFormat(TextureFormat format);

int GetTextureSizeAllowedMultiple(TextureFormat format);
int GetMinimumTextureMipSizeForFormat(TextureFormat format);
bool IsAlphaOnlyTextureFormat(TextureFormat format);

TextureFormat ConvertToAlphaTextureFormat(TextureFormat format);

bool HasAlphaTextureFormat(TextureFormat format);

//bool IsDepthRTFormat( RenderTextureFormat format );
//bool IsHalfRTFormat( RenderTextureFormat format );

const char* GetCompressionTypeString(TextureFormat format);
const char* GetTextureFormatString(TextureFormat format);
//const char* GetTextureColorSpaceString (TextureColorSpace colorSpace);

//TextureColorSpace ColorSpaceToTextureColorSpace(BuildTargetPlatform platform, ColorSpace colorSpace);

std::pair<int,int> RoundTextureDimensionsToBlocks(TextureFormat fmt, int w, int h);

NAMESPACE_RENDERCORE_END

#endif
