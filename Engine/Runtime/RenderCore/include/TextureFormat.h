#ifndef RENDER_CORE_TEXTURE_FORMAT_INCLKUDE_H
#define RENDER_CORE_TEXTURE_FORMAT_INCLKUDE_H

#include "RenderDefine.h"

NAMESPACE_RENDERCORE_BEGIN

// https://docs.vulkan.org/refpages/latest/refpages/source/VkFormat.html

//纹理格式枚举
typedef uint32_t TextureFormat;
enum
{
    kTexFormatInvalid = 0,
    kTexFormatAlpha8 = 1,
    kTexFormatLuma = 2,
    kTexFormatARGB4444 = 3,
    kTexFormatRGB24 = 4,
    kTexFormatRGBA8 = 5,
    kTexFormatARGB8 = 6,
    kTexFormatARGBFloat = 8, // only for internal use at runtime
    kTexFormatRGB565 = 9,
    kTexFormatRGBA5551 = 10,
    kTexFormatBGR24 = 11,
    kTexFormatRGBA4444 = 12,
    
    //srgb
    kTexFormatSRGB8 = 13,
    kTexFormatSRGB8_ALPHA8 = 14,

    kTexR10G10B10A2 = 15,
    
    
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
    // BC6H 分有符号/无符号两种（编码互不兼容）：
    //   UFLOAT 对应 GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT(0x8E8F)
    //   SFLOAT 对应 GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT(0x8E8E)
    kTexFormatBC6H_UFLOAT = 27,   // BC6H 无符号浮点（HDR 环境贴图默认）
    kTexFormatBC6H_SFLOAT = 28,   // BC6H 有符号浮点
    kTexFormatBC7_RGB = 29,
    kTexFormatBC7_SRGB = 30,

    // iPhone
    kTexFormatPVRTC_RGB2 = 31,
    kTexFormatPVRTC_RGBA2 = 32,

    kTexFormatPVRTC_RGB4 = 33,
    kTexFormatPVRTC_RGBA4 = 34,

    kTexFormatETC_RGB4 = 35,

    kTexFormatATC_RGB4 = 36,
    kTexFormatATC_RGBA8 = 37,

    // Pixels returned by iPhone camera
    kTexFormatBGRA32 = 38,

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

    // ==================== ASTC LDR ====================
    // 每个块尺寸都有 sRGB 与 UNORM(线性) 两种格式（ASTC 编码本身相同，仅视图/解码语义不同）。
    // 块尺寸覆盖 Vulkan/Metal ASTC LDR 全部 14 种（含非正方形块）。
    // 命名规则: kTexFormatASTC_<块宽>x<块高>_<SRGB|UNORM>
    // (旧 kTexFormatASTC_RGB_*x* / kTexFormatASTC_RGBA_*x* 已废弃移除；
    //  ASTC RGB 与 RGBA 内部编码一致，不再区分，由上层材质语义决定有无 alpha)
    kTexFormatASTC_4x4_SRGB = 52,
    kTexFormatASTC_4x4_UNORM = 53,
    kTexFormatASTC_5x4_SRGB = 54,
    kTexFormatASTC_5x4_UNORM = 55,
    kTexFormatASTC_5x5_SRGB = 56,
    kTexFormatASTC_5x5_UNORM = 57,
    kTexFormatASTC_6x5_SRGB = 58,
    kTexFormatASTC_6x5_UNORM = 59,
    kTexFormatASTC_6x6_SRGB = 60,
    kTexFormatASTC_6x6_UNORM = 61,
    kTexFormatASTC_8x5_SRGB = 62,
    kTexFormatASTC_8x5_UNORM = 63,
    kTexFormatASTC_8x6_SRGB = 64,
    kTexFormatASTC_8x6_UNORM = 65,
    kTexFormatASTC_8x8_SRGB = 66,
    kTexFormatASTC_8x8_UNORM = 67,
    kTexFormatASTC_10x5_SRGB = 68,
    kTexFormatASTC_10x5_UNORM = 69,
    kTexFormatASTC_10x6_SRGB = 70,
    kTexFormatASTC_10x6_UNORM = 71,
    kTexFormatASTC_10x8_SRGB = 72,
    kTexFormatASTC_10x8_UNORM = 73,
    kTexFormatASTC_10x10_SRGB = 74,
    kTexFormatASTC_10x10_UNORM = 75,
    kTexFormatASTC_12x10_SRGB = 76,
    kTexFormatASTC_12x10_UNORM = 77,
    kTexFormatASTC_12x12_SRGB = 78,
    kTexFormatASTC_12x12_UNORM = 79,

    //深度模板格式
    kTexFormatDepth16 = 80,            // 16 bit depth buffer
    kTexFormatDepth24 = 81,            // 24 bit depth buffer
    kTexFormatDepth32Float = 82,       // 32 bit float depth buffer
    
    kTexFormatDepth16Stencil8 = 83,
    kTexFormatDepth24Stencil8 = 84,
    kTexFormatDepth32FloatStencil8 = 85,
    
    
    //浮点格式纹理
    kTexFormatRGBA16Float = 86,
    kTexFormatRGBA32Float = 87,
    kTexFormatRG16Float = 88,

	kTexFormatR32Uint = 96,
	kTexFormatR32Sint = 97,
    kTexFormatR32Float = 98,

    kTexFormatRG32Uint = 99,
    kTexFormatRG32Sint = 100,
    kTexFormatRG32Float = 101,

    // 8-bit / 16-bit unsigned/integer formats
    // Useful for: virtual texture page tables, index buffers as textures,
    // packed coordinate storage, compute shader readback
    kTexFormatR8Uint   = 102,
    kTexFormatRG8Uint  = 103,
    kTexFormatRGBA8Uint= 104,
    kTexFormatR8Sint   = 105,
    kTexFormatRG8Sint  = 106,
    kTexFormatRGBA8Sint= 107,
    kTexFormatR16Uint  = 108,
    kTexFormatRG16Uint = 109,
    kTexFormatRGBA16Uint=110,
    kTexFormatR16Sint  = 111,
    kTexFormatRG16Sint = 112,
    kTexFormatRGBA16Sint=113,

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
	// ASTC LDR 全部块尺寸 × {SRGB, UNORM} 连续排列（kTexFormatASTC_4x4_SRGB .. kTexFormatASTC_12x12_UNORM）
	return format >= kTexFormatASTC_4x4_SRGB && format <= kTexFormatASTC_12x12_UNORM;
}

// ASTC sRGB 变体（每尺寸的 SRGB 在前、UNORM 在后，间隔 1）
inline bool IsASTCSRGBFormat(TextureFormat format)
{
	return IsCompressedASTCTextureFormat(format) && ((format - kTexFormatASTC_4x4_SRGB) & 1) == 0;
}

// ASTC UNORM(线性) 变体
inline bool IsASTCUNORMFormat(TextureFormat format)
{
	return IsCompressedASTCTextureFormat(format) && !IsASTCSRGBFormat(format);
}

inline bool IsCompressedBCTextureFormat(TextureFormat format)
{
	return format >= kTexFormatBC6H_UFLOAT && format <= kTexFormatBC7_SRGB;
}

inline bool IsAnyCompressedTextureFormat(TextureFormat format)
{
	return IsCompressedDXTTextureFormat(format) || IsCompressedBCTextureFormat(format)
			|| IsCompressedPVRTCTextureFormat(format)
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

// ==================== 压缩格式块几何信息 ====================
// 块压缩格式按固定大小的“块”(block) 组织数据（如 BC/DXT/ETC=4x4，ASTC 可为
// 5x5/6x6/8x8/10x10/12x12，PVRTC 2bpp=8x4 等）。计算 mip 链 / 上传 bytesPerRow 时
// 必须使用真实块尺寸，不能写死 4x4，否则 ASTC/PVRTC 等会算错块行数导致越界。
struct TextureBlockInfo
{
    uint32_t blockWidth  = 1;   // 块宽（texel）
    uint32_t blockHeight = 1;   // 块高（texel）
    uint32_t bytesPerBlock = 0; // 每个块的字节数（0 = 非压缩格式）
};

// 返回压缩格式的块尺寸与每块字节数；非压缩格式返回 bytesPerBlock=0。
TextureBlockInfo GetCompressedTextureBlockInfo(TextureFormat format);

// ==================== ASTC 辅助 ====================
// ASTC LDR 支持的全部块尺寸数量（正方形 + 非正方形）
constexpr uint32_t kASTCBlockSizeCount = 14;

// 把 ASTC 块尺寸 + 颜色空间映射为引擎 ASTC TextureFormat；非法尺寸返回 kTexFormatInvalid。
// 供 GL/Vk 格式转换（ImageTextureUtil / 后端映射）与资产烘焙共用，避免各处重复维护尺寸表。
TextureFormat ConvertASTCBlockToEngineFormat(uint32_t blockWidth, uint32_t blockHeight, bool sRGB);

// 从引擎 ASTC TextureFormat 反查块尺寸（与 GetCompressedTextureBlockInfo 一致，供名称等使用）
bool GetASTCBlockSizeFromFormat(TextureFormat format, uint32_t& outBlockWidth, uint32_t& outBlockHeight);

// 返回某高度（像素行）对应的块行数；非压缩格式返回原值（每像素一行）。
inline uint32_t GetBlockRowCount(TextureFormat format, uint32_t pixelHeight)
{
    TextureBlockInfo info = GetCompressedTextureBlockInfo(format);
    if (info.bytesPerBlock == 0 || info.blockHeight == 0)
    {
        return pixelHeight;
    }
    return (pixelHeight + info.blockHeight - 1) / info.blockHeight;
}

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
