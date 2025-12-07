#ifndef RENDER_CORE_TEXTURE_FORMAT_INCLKUDE_H
#define RENDER_CORE_TEXTURE_FORMAT_INCLKUDE_H

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

// https://docs.vulkan.org/refpages/latest/refpages/source/VkFormat.html

#if 0
typedef enum VkFormat {
    VK_FORMAT_UNDEFINED = 0,
    VK_FORMAT_R4G4_UNORM_PACK8 = 1,
    VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
    VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
    VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
    VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
    VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
    VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
    VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
    VK_FORMAT_R8_UNORM = 9,
    VK_FORMAT_R8_SNORM = 10,
    VK_FORMAT_R8_USCALED = 11,
    VK_FORMAT_R8_SSCALED = 12,
    VK_FORMAT_R8_UINT = 13,
    VK_FORMAT_R8_SINT = 14,
    VK_FORMAT_R8_SRGB = 15,
    VK_FORMAT_R8G8_UNORM = 16,
    VK_FORMAT_R8G8_SNORM = 17,
    VK_FORMAT_R8G8_USCALED = 18,
    VK_FORMAT_R8G8_SSCALED = 19,
    VK_FORMAT_R8G8_UINT = 20,
    VK_FORMAT_R8G8_SINT = 21,
    VK_FORMAT_R8G8_SRGB = 22,
    VK_FORMAT_R8G8B8_UNORM = 23,
    VK_FORMAT_R8G8B8_SNORM = 24,
    VK_FORMAT_R8G8B8_USCALED = 25,
    VK_FORMAT_R8G8B8_SSCALED = 26,
    VK_FORMAT_R8G8B8_UINT = 27,
    VK_FORMAT_R8G8B8_SINT = 28,
    VK_FORMAT_R8G8B8_SRGB = 29,
    VK_FORMAT_B8G8R8_UNORM = 30,
    VK_FORMAT_B8G8R8_SNORM = 31,
    VK_FORMAT_B8G8R8_USCALED = 32,
    VK_FORMAT_B8G8R8_SSCALED = 33,
    VK_FORMAT_B8G8R8_UINT = 34,
    VK_FORMAT_B8G8R8_SINT = 35,
    VK_FORMAT_B8G8R8_SRGB = 36,
    VK_FORMAT_R8G8B8A8_UNORM = 37,
    VK_FORMAT_R8G8B8A8_SNORM = 38,
    VK_FORMAT_R8G8B8A8_USCALED = 39,
    VK_FORMAT_R8G8B8A8_SSCALED = 40,
    VK_FORMAT_R8G8B8A8_UINT = 41,
    VK_FORMAT_R8G8B8A8_SINT = 42,
    VK_FORMAT_R8G8B8A8_SRGB = 43,
    VK_FORMAT_B8G8R8A8_UNORM = 44,
    VK_FORMAT_B8G8R8A8_SNORM = 45,
    VK_FORMAT_B8G8R8A8_USCALED = 46,
    VK_FORMAT_B8G8R8A8_SSCALED = 47,
    VK_FORMAT_B8G8R8A8_UINT = 48,
    VK_FORMAT_B8G8R8A8_SINT = 49,
    VK_FORMAT_B8G8R8A8_SRGB = 50,
    VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
    VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
    VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
    VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
    VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
    VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
    VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
    VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
    VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
    VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
    VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
    VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
    VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
    VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
    VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
    VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
    VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
    VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
    VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
    VK_FORMAT_R16_UNORM = 70,
    VK_FORMAT_R16_SNORM = 71,
    VK_FORMAT_R16_USCALED = 72,
    VK_FORMAT_R16_SSCALED = 73,
    VK_FORMAT_R16_UINT = 74,
    VK_FORMAT_R16_SINT = 75,
    VK_FORMAT_R16_SFLOAT = 76,
    VK_FORMAT_R16G16_UNORM = 77,
    VK_FORMAT_R16G16_SNORM = 78,
    VK_FORMAT_R16G16_USCALED = 79,
    VK_FORMAT_R16G16_SSCALED = 80,
    VK_FORMAT_R16G16_UINT = 81,
    VK_FORMAT_R16G16_SINT = 82,
    VK_FORMAT_R16G16_SFLOAT = 83,
    VK_FORMAT_R16G16B16_UNORM = 84,
    VK_FORMAT_R16G16B16_SNORM = 85,
    VK_FORMAT_R16G16B16_USCALED = 86,
    VK_FORMAT_R16G16B16_SSCALED = 87,
    VK_FORMAT_R16G16B16_UINT = 88,
    VK_FORMAT_R16G16B16_SINT = 89,
    VK_FORMAT_R16G16B16_SFLOAT = 90,
    VK_FORMAT_R16G16B16A16_UNORM = 91,
    VK_FORMAT_R16G16B16A16_SNORM = 92,
    VK_FORMAT_R16G16B16A16_USCALED = 93,
    VK_FORMAT_R16G16B16A16_SSCALED = 94,
    VK_FORMAT_R16G16B16A16_UINT = 95,
    VK_FORMAT_R16G16B16A16_SINT = 96,
    VK_FORMAT_R16G16B16A16_SFLOAT = 97,
    VK_FORMAT_R32_UINT = 98,
    VK_FORMAT_R32_SINT = 99,
    VK_FORMAT_R32_SFLOAT = 100,
    VK_FORMAT_R32G32_UINT = 101,
    VK_FORMAT_R32G32_SINT = 102,
    VK_FORMAT_R32G32_SFLOAT = 103,
    VK_FORMAT_R32G32B32_UINT = 104,
    VK_FORMAT_R32G32B32_SINT = 105,
    VK_FORMAT_R32G32B32_SFLOAT = 106,
    VK_FORMAT_R32G32B32A32_UINT = 107,
    VK_FORMAT_R32G32B32A32_SINT = 108,
    VK_FORMAT_R32G32B32A32_SFLOAT = 109
} VkFormat;

#endif

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
    kTexFormatDXT1_RGB = 21,
    kTexFormatDXT1_SRGB = 22,
    kTexFormatDXT3_RGB = 23,
    kTexFormatDXT3_SRGB = 24,
    kTexFormatDXT5_RGB = 25,
    kTexFormatDXT5_SRGB = 26,

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

    kTexFormatR32Float = 100,

    kTexFormatRG32Uint = 101,
    kTexFormatRG32Sint = 102,
    kTexFormatRG32Float = 103,

    kTexFormatTotalCount    = 1000 // keep this last!
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
	return format >= kTexFormatDXT1_RGB && format <= kTexFormatDXT5_SRGB;
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
	return IsCompressedDXTTextureFormat(format) || IsCompressedPVRTCTextureFormat(format)
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

const char* GetCompressionTypeString(TextureFormat format);
const char* GetTextureFormatString(TextureFormat format);

std::pair<int,int> RoundTextureDimensionsToBlocks(TextureFormat fmt, int w, int h);

enum TextureType 
{
    TextureType_Unkown =       -1,
    TextureType_2D =            0,
    TextureType_3D =            1,
    TextureType_2D_ARRAY =      2,
    TextureType_CUBE =          3
};

NAMESPACE_RENDERCORE_END

#endif
