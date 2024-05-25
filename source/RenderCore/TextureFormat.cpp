
#include "TextureFormat.h"
#include <assert.h>

NAMESPACE_RENDERCORE_BEGIN

// NOTE: match indices in kTexFormat* enum!

const static int kTextureByteTable[kTexFormatTotalCount] =
{
	0,
	1,	// kTexFormatAlpha8
	2,	// kTexFormatARGB4444
	3,  // kTexFormatRGB24
	4,	// kTexFormatRGBA32
	4,	// kTexFormatARGB32
	16, // kTexFormatARGBFloat
	2,  // kTexFormatRGB565
	3,  // kTexFormatBGR24
	2,  // kTexFormatAlphaLum16
	0,  // kTexFormatDXT1 (Depends on width, height, depth)
	0,  // kTexFormatDXT3 (Depends on width, height, depth)
	0,  // kTexFormatDXT5 (Depends on width, height, depth)
	2,	// kTexFormatRGBA4444
};

uint32_t GetBytesFromTextureFormat (TextureFormat inFormat)
{
    assert (inFormat < kTexFormatDXT1 || inFormat == kTexFormatBGRA32 || inFormat == kTexFormatRGBA4444);
	return (inFormat == kTexFormatBGRA32) ? 4 : kTextureByteTable[inFormat];
}

uint32_t GetMaxBytesPerPixel (TextureFormat inFormat)
{
    assert (GetBytesFromTextureFormat (inFormat) <= kTextureByteTable[kTexFormatARGBFloat]);
	return kTextureByteTable[kTexFormatARGBFloat];
}

int GetRowBytesFromWidthAndFormat (int width, TextureFormat inFormat)
{
    assert (inFormat < kTexFormatDXT1 || inFormat == kTexFormatBGRA32 || inFormat == kTexFormatRGBA4444);
	return GetBytesFromTextureFormat (inFormat) * width;
}

bool IsValidTextureFormat (TextureFormat format)
{
	if ((format >= kTexFormatAlpha8 && format <= kTexFormatRGBA4444) ||
		IsCompressedPVRTCTextureFormat(format) ||
		IsCompressedETCTextureFormat(format) ||
		IsCompressedATCTextureFormat(format) ||
		IsCompressedETC2TextureFormat(format) ||
		IsCompressedASTCTextureFormat(format) ||
		IsCompressedEACTextureFormat(format) ||
		format == kTexFormatBGRA32)
		return true;
	else
		return false;
}

int GetTextureSizeAllowedMultiple( TextureFormat format )
{
	if (IsCompressedDXTTextureFormat(format) || IsCompressedATCTextureFormat(format) || IsCompressedETCTextureFormat(format)
		 || IsCompressedETC2TextureFormat(format) || IsCompressedEACTextureFormat(format))
		return 4;
	else
		return 1;
}

int GetMinimumTextureMipSizeForFormat( TextureFormat format )
{
	if (format == kTexFormatPVRTC_RGBA2 || format == kTexFormatPVRTC_RGB2)
		return 16;
	else if (format == kTexFormatPVRTC_RGBA4 || format == kTexFormatPVRTC_RGB4)
		return 8;
	else if (IsCompressedETCTextureFormat(format) || IsCompressedETC2TextureFormat(format) || IsCompressedEACTextureFormat(format))
		return 4;
	else if (IsCompressedATCTextureFormat(format))
		return 4;
	else if (IsCompressedASTCTextureFormat(format))
		return 1;
	else if (IsCompressedDXTTextureFormat(format))
		return 4;
	else
		return 1;
}

bool IsAlphaOnlyTextureFormat( TextureFormat format )
{
	return format == kTexFormatAlpha8;
}


TextureFormat ConvertToAlphaTextureFormat (TextureFormat format)
{
	if (format == kTexFormatRGB24 || format == kTexFormatBGR24)
		return kTexFormatARGB32;
	else if (format == kTexFormatRGB565)
		return kTexFormatARGB4444;
	else if (format == kTexFormatDXT1)
		return kTexFormatDXT5;
	else if (format == kTexFormatPVRTC_RGB2)
		return kTexFormatPVRTC_RGBA2;
	else if (format == kTexFormatPVRTC_RGB4)
		return kTexFormatPVRTC_RGBA4;
	else if (format == kTexFormatETC_RGB4)
		return kTexFormatARGB4444;
	else if (format == kTexFormatATC_RGB4)
		return kTexFormatATC_RGBA8;
	else if (format == kTexFormatETC2_RGB)
		return kTexFormatETC2_RGBA8;
	else
		return format;
}

bool HasAlphaTextureFormat( TextureFormat format )
{
	return format == kTexFormatAlpha8 || format == kTexFormatARGB4444 || format == kTexFormatRGBA4444 || format == kTexFormatRGBA32 || format == kTexFormatARGB32
	|| format == kTexFormatARGBFloat || format == kTexFormatAlphaLum16 || format == kTexFormatDXT5 || format == kTexFormatDXT3
	|| format == kTexFormatPVRTC_RGBA2 || format == kTexFormatPVRTC_RGBA4 || format == kTexFormatATC_RGBA8 || format == kTexFormatBGRA32
	|| format == kTexFormatETC2_RGBA1 || format == kTexFormatETC2_RGBA8 || (format >= kTexFormatASTC_RGBA_4x4 && format <= kTexFormatASTC_RGBA_12x12);
}

//bool IsDepthRTFormat( RenderTextureFormat format )
//{
//	return format == kRTFormatDepth || format == kRTFormatShadowMap;
//}
//
//bool IsHalfRTFormat( RenderTextureFormat format )
//{
//	return format == kRTFormatARGBHalf || format == kRTFormatRGHalf || format == kRTFormatRHalf || IsDepthRTFormat(format);
//}

const char* GetCompressionTypeString (TextureFormat format)
{
	// Shortcut here, no sense in typing all block sizes in switchcase below
	if(IsCompressedASTCTextureFormat(format))
		return "ASTC";

	switch (format)
	{
		case kTexFormatDXT1: return "DXT1";
		case kTexFormatDXT3: return "DXT3";
		case kTexFormatDXT5: return "DXT5";
		case kTexFormatETC_RGB4: return "ETC1";
		case kTexFormatETC2_RGB:
		case kTexFormatETC2_RGBA1:
		case kTexFormatETC2_RGBA8: return "ETC2";
		case kTexFormatEAC_R:
		case kTexFormatEAC_R_SIGNED:
		case kTexFormatEAC_RG:
		case kTexFormatEAC_RG_SIGNED: return "EAC";
		case kTexFormatPVRTC_RGB2:
		case kTexFormatPVRTC_RGBA2:
		case kTexFormatPVRTC_RGB4:
		case kTexFormatPVRTC_RGBA4: return "PVRTC";
		case kTexFormatATC_RGB4:
		case kTexFormatATC_RGBA8: return "ATC";
		default:
			return "Uncompressed";
	}
}

const char* GetTextureFormatString(TextureFormat format)
{
	switch (format)
	{
		case kTexFormatAlpha8: return "Alpha 8";
		case kTexFormatARGB4444: return "ARGB 16 bit";
		case kTexFormatRGBA4444: return "RGBA 16 bit";
		case kTexFormatRGB24: return "RGB 24 bit";
		case kTexFormatRGBA32: return "RGBA 32 bit";
		case kTexFormatARGB32: return "ARGB 32 bit";
		case kTexFormatARGBFloat: return "ARGB float";
		case kTexFormatRGB565: return "RGB 16 bit";
		case kTexFormatBGR24: return "BGR 24 bit";
		case kTexFormatAlphaLum16: return "Alpha 16 bit";
		case kTexFormatDXT1: return "RGB Compressed DXT1";
		case kTexFormatDXT3: return "RGBA Compressed DXT3";
		case kTexFormatDXT5: return "RGBA Compressed DXT5";

		// gles
		case kTexFormatPVRTC_RGB2: return "RGB Compressed PVRTC 2 bits";
		case kTexFormatPVRTC_RGBA2: return "RGBA Compressed PVRTC 2 bits";
		case kTexFormatPVRTC_RGB4: return "RGB Compressed PVRTC 4 bits";
		case kTexFormatPVRTC_RGBA4: return "RGBA Compressed PVRTC 4 bits";
		case kTexFormatETC_RGB4: return "RGB Compressed ETC 4 bits";
		case kTexFormatATC_RGB4: return "RGB Compressed ATC 4 bits";
		case kTexFormatATC_RGBA8: return "RGBA Compressed ATC 8 bits";

		case kTexFormatETC2_RGB: return "RGB Compressed ETC2 4 bits";
		case kTexFormatETC2_RGBA1: return "RGB + 1-bit Alpha Compressed ETC2 4 bits";
		case kTexFormatETC2_RGBA8: return "RGBA Compressed ETC2 8 bits";
		case kTexFormatEAC_R: return "11-bit R Compressed EAC 4 bit";
		case kTexFormatEAC_R_SIGNED: return "11-bit signed R Compressed EAC 4 bit";
		case kTexFormatEAC_RG: return "11-bit RG Compressed EAC 8 bit";
		case kTexFormatEAC_RG_SIGNED: return "11-bit signed RG Compressed EAC 8 bit";

#define STR_(x) #x
#define STR(x) STR_(x)
#define DO_ASTC(bx,by) case kTexFormatASTC_RGB_##bx##x##by : return "RGB Compressed ASTC " STR(bx) "x" STR(by) " block"; case kTexFormatASTC_RGBA_##bx##x##by : return "RGBA Compressed ASTC " STR(bx) "x" STR(by) " block"

		DO_ASTC(4, 4);
		DO_ASTC(5, 5);
		DO_ASTC(6, 6);
		DO_ASTC(8, 8);
		DO_ASTC(10, 10);
		DO_ASTC(12, 12);

#undef DO_ASTC
#undef STR
#undef STR_

		case kTexFormatBGRA32: return "BGRA 32 bit";

		default:
		return "Unsupported";
	}
}

//const char* GetTextureColorSpaceString (TextureColorSpace colorSpace)
//{
//	switch (colorSpace)
//	{
//		case kTexColorSpaceLinear: return "Linear";
//		case kTexColorSpaceSRGB: return "sRGB";
//		case kTexColorSpaceSRGBXenon: return "sRGB (Xenon)";
//		default: return "Unsupported";
//	}
//}
//
//TextureColorSpace ColorSpaceToTextureColorSpace(BuildTargetPlatform platform, ColorSpace colorSpace)
//{
//	if (colorSpace == kGammaColorSpace)
//		return (platform == kBuildXBOX360) ? kTexColorSpaceSRGBXenon : kTexColorSpaceSRGB;
//	else
//		return kTexColorSpaceLinear;
//}

NAMESPACE_RENDERCORE_END
