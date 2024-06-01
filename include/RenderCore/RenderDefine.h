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

// 命名空间宏定义
#define NAMESPACE_RENDERCORE_BEGIN        namespace RenderCore {
#define NAMESPACE_RENDERCORE_END            }
#define USING_NS_RENDERCORE using namespace RenderCore;

NAMESPACE_RENDERCORE_BEGIN

typedef void* ViewHandle;

typedef enum RenderDeviceType
{
    GLES,
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
    MIN_NEAREST = 0,
    MIN_LINEAR = 1,
    MIN_NEAREST_MIPMAP_NEAREST = 2,
    MIN_LINEAR_MIPMAP_NEAREST = 3,
    MIN_NEAREST_MIPMAP_LINEAR = 4,
    MIN_LINEAR_MIPMAP_LINEAR = 5
};

//放大采样器
enum SamplerMagFilter
{
    MAG_NEAREST,              //最邻近
    MAG_LINEAR,              //双线性
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
enum TextureUsage
{
    TextureUsageUnknown         = 0x0000,
    TextureUsageShaderRead      = 0x0001,
    TextureUsageShaderWrite     = 0x0002,
    TextureUsageRenderTarget    = 0x0004,
};

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

NAMESPACE_RENDERCORE_END

#endif /* RENDERENGINE_TYPEDEFINE_INCLUDE_H */
