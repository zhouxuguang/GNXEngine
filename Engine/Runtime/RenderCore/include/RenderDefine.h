//
//  RenderDefine.h
//  GNXEngine
//
//  Created by zhouxuguang on 2019/7/5.
//  Copyright © 2019 zhouxuguang. All rights reserved.
//

#ifndef RENDERENGINE_TYPEDEFINE_INCLUDE_H
#define RENDERENGINE_TYPEDEFINE_INCLUDE_H

#include <memory>
#include <vector>
#include <string>

#if _WIN32
	#ifdef __GNUC__
		#define RENDERCORE_API __attribute__((dllexport))
	#else
		#define RENDERCORE_API __declspec(dllexport)
	#endif
	#define RENDERCORE_API_HIDE
#else
	#if __GNUC__>=4
		#define RENDERCORE_API __attribute__((visibility("default")))
		#define RENDERCORE_API_HIDE __attribute__ ((visibility("hidden")))
	#else
		#define RENDERCORE_API_HIDE
		#define RENDERCORE_API
	#endif
#endif

// 命名空间宏定义
#define NAMESPACE_RENDERCORE_BEGIN        namespace RenderCore {
#define NAMESPACE_RENDERCORE_END            }
#define USING_NS_RENDERCORE using namespace RenderCore;

NAMESPACE_RENDERCORE_BEGIN

typedef void* ViewHandle;

typedef enum RenderDeviceType
{
    METAL,
    VULKAN,
} RenderDeviceType;

//shader阶段的枚举
enum ShaderStage
{
    ShaderStage_Vertex = 0,
    ShaderStage_Fragment = 1,
    ShaderStage_Compute = 2,
    ShaderStage_Max = 3,
};

//清除缓冲区的模式
enum ClearBufferMode
{
    COLOR_BUFFER = 1,                                              //颜色缓冲
    DEPTH_BUFFER = 2,                                              //深度缓冲
    STENCIL_BUFFER = 4,                                            //模板缓冲
    COLORDEPTH_BUFFER = COLOR_BUFFER | DEPTH_BUFFER,               //颜色和深度缓冲
    ALL_BUFFER = COLOR_BUFFER | DEPTH_BUFFER | STENCIL_BUFFER      //全部缓冲
};

enum DrawBufferMode
{
    COLOR_BUFFER_NONE = 0,
    COLOR_BUFFER_ATTCH0 = 1,
    COLOR_BUFFER_ATTCH1 = 2,
    COLOR_BUFFER_ATTCH2 = 3,
    COLOR_BUFFER_ATTCH3 = 4,
    COLOR_BUFFER_ATTCH4 = 5,
    COLOR_BUFFER_ATTCH5 = 6,
    COLOR_BUFFER_ATTCH6 = 7,
    COLOR_BUFFER_ATTCH7 = 8,
};

//图元类型
enum PrimitiveMode
{
    PrimitiveMode_POINTS = 0,
    PrimitiveMode_LINES = 1,
    PrimitiveMode_LINE_STRIP = 2,
    PrimitiveMode_TRIANGLES = 3,
    PrimitiveMode_TRIANGLE_STRIP = 4
};

//存储模式
enum StorageMode
{
    StorageModeShared,  //GPU和CPU都可访问
    StorageModePrivate, //GPU访问
};

//索引类型
enum IndexType
{
    IndexType_UShort,
    IndexType_UInt,
};

//比较函数
enum CompareFunction
{
    CompareFunctionNever = 0,                    //从不比较
    CompareFunctionLess = 1,                     //小于
    CompareFunctionEqual = 2,                    //相等
    CompareFunctionLessThanOrEqual = 3,          //小于或者等于
    CompareFunctionGreater = 4,                  //大于
    CompareFunctionNotEqual = 5,                 //不等于
    CompareFunctionGreaterThanOrEqual = 6,       //大于或者等于
    CompareFunctionAlways = 7,                   //总是
};

//缩小采样器
enum SamplerMinFilter
{
    MIN_NEAREST = 0,   //最邻近
    MIN_LINEAR = 1     //双线性
};

//放大采样器
enum SamplerMagFilter
{
    MAG_NEAREST = 0,              //最邻近
    MAG_LINEAR = 1,              //双线性
};

//mipmap采样器
enum SamplerMipFilter
{
	MIN_NEAREST_MIPMAP_NEAREST = 0,
	MIN_LINEAR_MIPMAP_NEAREST = 1,
	MIN_NEAREST_MIPMAP_LINEAR = 2,
	MIN_LINEAR_MIPMAP_LINEAR = 3
};

//纹理采样地址环绕模式
enum SamplerWrapMode
{
    CLAMP_TO_EDGE,                  //截断模式
    REPEAT,                 //重复模式
    MIRRORED_REPEAT,         //镜像重复模式
};

// 采样器比较模式
enum SamplerCompareMode
{
    NONE = 0,
    COMPARE_TO_TEXTURE = 1
};

// 纹理使用的模式
enum class TextureUsage : uint32_t
{
    TextureUsageUnknown         = 0x0000,
    TextureUsageShaderRead      = 0x0001,
    TextureUsageShaderWrite     = 0x0002,
    TextureUsageRenderTarget    = 0x0004,
};

// TextureUsage 位运算符重载
constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) noexcept
{
    return static_cast<TextureUsage>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

constexpr TextureUsage operator&(TextureUsage lhs, TextureUsage rhs) noexcept
{
    return static_cast<TextureUsage>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

constexpr TextureUsage operator^(TextureUsage lhs, TextureUsage rhs) noexcept
{
    return static_cast<TextureUsage>(
        static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs));
}

constexpr TextureUsage operator~(TextureUsage value) noexcept
{
    return static_cast<TextureUsage>(~static_cast<uint32_t>(value));
}

constexpr TextureUsage& operator|=(TextureUsage& lhs, TextureUsage rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr TextureUsage& operator&=(TextureUsage& lhs, TextureUsage rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

constexpr TextureUsage& operator^=(TextureUsage& lhs, TextureUsage rhs) noexcept
{
    lhs = lhs ^ rhs;
    return lhs;
}

// 模板函数：检查是否包含特定用途
template<TextureUsage Usage>
constexpr bool HasUsage(TextureUsage value) noexcept
{
    return (value & Usage) == Usage;
}

// 模板函数：检查是否有任何用途
template<TextureUsage... Usages>
constexpr bool HasAnyUsage(TextureUsage value) noexcept
{
    bool result = false;
    ((result = result || ((value & Usages) != TextureUsage::TextureUsageUnknown)), ...);
    return result;
}

// 模板函数：添加用途
template<TextureUsage... Usages>
constexpr TextureUsage AddUsages() noexcept
{
    TextureUsage result = TextureUsage::TextureUsageUnknown;
    ((result = result | Usages), ...);
    return result;
}

//模板测试的操作
enum StencilOperation
{
    StencilOperationKeep = 0,
    StencilOperationZero = 1,
    StencilOperationReplace = 2,
    StencilOperationIncrementClamp = 3,
    StencilOperationDecrementClamp = 4,
    StencilOperationInvert = 5,
    StencilOperationIncrementWrap = 6,
    StencilOperationDecrementWrap = 7,
};

//颜色混合因子
enum BlendFactor
{
    BlendFactorZero = 0,
    BlendFactorOne = 1,
    BlendFactorSourceColor = 2,
    BlendFactorOneMinusSourceColor = 3,
    BlendFactorSourceAlpha = 4,
    BlendFactorOneMinusSourceAlpha = 5,
    BlendFactorDestinationColor = 6,
    BlendFactorOneMinusDestinationColor = 7,
    BlendFactorDestinationAlpha = 8,
    BlendFactorOneMinusDestinationAlpha = 9,
    BlendFactorSourceAlphaSaturated = 10,
    BlendFactorBlendColor = 11,
    BlendFactorOneMinusBlendColor = 12,
    BlendFactorBlendAlpha = 13,
    BlendFactorOneMinusBlendAlpha = 14
};

//颜色混合的计算方式
enum BlendEquation
{
    BlendEquationAdd = 0,
    BlendEquationSubtract,
    BlendEquationReverseSubtract,
    BlendEquationMinimum,
    BlendEquationMaximum,
};

//背面剔除的方式
enum CullMode
{
    CullModeNone = 0,
    CullModeFront = 1,
    CullModeBack = 2,
};

//=========================================

enum VertexFormat
{
    VertexFormatInvalid = 0,
    
    VertexFormatUChar = 1,
    VertexFormatUChar2 = 2,
    VertexFormatUChar3 = 3,
    VertexFormatUChar4 = 4,
    
    VertexFormatChar = 5,
    VertexFormatChar2 = 6,
    VertexFormatChar3 = 7,
    VertexFormatChar4 = 8,
   
    VertexFormatUShort = 9,
    VertexFormatUShort2 = 10,
    VertexFormatUShort3 = 11,
    VertexFormatUShort4 = 12,
    
    VertexFormatShort = 13,
    VertexFormatShort2 = 14,
    VertexFormatShort3 = 15,
    VertexFormatShort4 = 16,
    
    VertexFormatFloat = 28,
    VertexFormatFloat2 = 29,
    VertexFormatFloat3 = 30,
    VertexFormatFloat4 = 31,
    
    VertexFormatInt = 32,
    VertexFormatInt2 = 33,
    VertexFormatInt3 = 34,
    VertexFormatInt4 = 35,
    
    VertexFormatUInt = 36,
    VertexFormatUInt2 = 37,
    VertexFormatUInt3 = 38,
    VertexFormatUInt4 = 39,
    
    VertexFormatHalfFloat = 40,
    VertexFormatHalfFloat2 = 41,
    VertexFormatHalfFloat3 = 42,
    VertexFormatHalfFloat4 = 43,
};

enum VertextStepFunc
{
    VertexStepFunctionPerVertex = 1,
};

enum ColorWriteMask
{
    ColorWriteMaskNone  = 0,
    ColorWriteMaskRed   = 0x1 << 3,
    ColorWriteMaskGreen = 0x1 << 2,
    ColorWriteMaskBlue  = 0x1 << 1,
    ColorWriteMaskAlpha = 0x1 << 0,
    ColorWriteMaskAll   = 0xf
};

// Reverse-Z 深度配置
// Reverse-Z 通过反转深度值映射（近处=1，远处=0）来提高深度精度
struct RENDERCORE_API DepthConfig
{
    // 全局 Reverse-Z 开关，默认启用
    static bool UseReverseZ;
    
    // 获取默认深度比较函数
    static CompareFunction GetDefaultDepthCompareFunc() 
    {
        return UseReverseZ ? CompareFunctionGreater : CompareFunctionLess;
    }
    
    // 获取天空盒深度比较函数
    // 天空盒渲染在深度最大处，Reverse-Z 时使用 GreaterThanOrEqual
    static CompareFunction GetSkyboxDepthCompareFunc() 
    {
        return UseReverseZ ? CompareFunctionGreaterThanOrEqual : CompareFunctionLessThanOrEqual;
    }
    
    // 获取默认深度清除值
    // Reverse-Z: 清除为 0.0（远处）
    // 传统 Z: 清除为 1.0（远处）
    static float GetDefaultClearDepth() 
    {
        return UseReverseZ ? 0.0f : 1.0f;
    }
};

enum CubemapFace
{
    kCubeFaceUnknown = -1,
    kCubeFacePX = 0,
    kCubeFaceNX,
    kCubeFacePY,
    kCubeFaceNY,
    kCubeFacePZ,
    kCubeFaceNZ,
};

//矩形区域定义
struct Rect2D
{
    Rect2D()
    {
    }
    
    Rect2D(int offsetX, int offsetY, int width, int height)
    {
        this->offsetX = offsetX;
        this->offsetY = offsetY;
        this->width = width;
        this->height = height;
    }
    
    int offsetX = 0;
    int offsetY = 0;
    int width;
    int height;
};

/**
 * 资源访问类型枚举
 */
enum class ResourceAccessType : uint32_t
{
    Unknown = 0,
    ComputeShaderRead = 1 << 0,
    ComputeShaderWrite = 1 << 1,
    ShaderRead = 1 << 2,
    ColorAttachment = 1 << 3,
    DepthStencilAttachment = 1 << 4,
    TransferSrc = 1 << 5,
    TransferDst = 1 << 6,
};

inline ResourceAccessType operator|(ResourceAccessType a, ResourceAccessType b)
{
    return static_cast<ResourceAccessType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ResourceAccessType operator&(ResourceAccessType a, ResourceAccessType b)
{
    return static_cast<ResourceAccessType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}


//non indexed
struct DrawIndirectCommand
{
	uint32_t    vertexCount;
	uint32_t    instanceCount;
	uint32_t    firstVertex;
	uint32_t    firstInstance;
};

//indexed 
struct DrawIndexedIndirectCommand 
{
	uint32_t    indexCount;
	uint32_t    instanceCount;
	uint32_t    firstIndex;
	int32_t     vertexOffset;
	uint32_t    firstInstance;
};

NAMESPACE_RENDERCORE_END

#endif /* RENDERENGINE_TYPEDEFINE_INCLUDE_H */
