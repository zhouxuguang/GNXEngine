//
//  DX12Util.cpp
//  rendercore
//

#include "DX12Util.h"

NAMESPACE_RENDERCORE_BEGIN

const char* DX12HResultToString(HRESULT hr)
{
    // 用 FormatMessage 取系统错误文本，避免引入 comsuppw.lib 依赖。
    // 仅在失败路径调用，性能无关。
    static thread_local char s_buffer[512] = {0};

    wchar_t* wmsg = nullptr;
    const DWORD len = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, (DWORD)hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&wmsg, 0, nullptr);

    if (len == 0 || wmsg == nullptr)
    {
        snprintf(s_buffer, sizeof(s_buffer), "HRESULT 0x%08X", (unsigned)hr);
        return s_buffer;
    }

    // 宽字符转窄字符（错误信息以 ASCII 为主，非 ASCII 字符替换为 '?'）
    size_t i = 0;
    for (; i < sizeof(s_buffer) - 1 && wmsg[i] != L'\0' && i < (size_t)len; ++i)
    {
        const wchar_t c = wmsg[i];
        s_buffer[i] = (c > 0 && c < 0x80 && c != L'\r' && c != L'\n') ? (char)c : ' ';
    }
    s_buffer[i] = '\0';
    LocalFree(wmsg);

    return s_buffer;
}

// ============================================================================
// 纹理格式
// ============================================================================

DXGI_FORMAT DX12Util::ConvertTextureFormat(TextureFormat texFormat)
{
    switch (texFormat)
    {
        // ---- 8 位单通道 ----
        case kTexFormatAlpha8:
        case kTexFormatLuma:
            return DXGI_FORMAT_R8_UNORM;

        // ---- 16 位打包 ----
        case kTexFormatRGBA4444:
            return DXGI_FORMAT_B4G4R4A4_UNORM;
        case kTexFormatRGBA5551:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case kTexFormatRGB565:
            return DXGI_FORMAT_B5G6R5_UNORM;

        // ---- 双通道 ----
        case kTexFormatAlphaLum16:
            return DXGI_FORMAT_R8G8_UNORM;

        // ---- 8 位四通道 ----
        case kTexFormatRGBA8:
        case kTexFormatARGB8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        // Vulkan 后端把 RGB24/BGR24 归一化为 R8G8B8A8（见 ImageTextureUtil 的
        // RGB8→RGBA8 预转换），这里保持同样的“向上取整到 4 通道”策略。
        case kTexFormatRGB24:
        case kTexFormatBGR24:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case kTexFormatSRGB8:
            // DXGI 没有 24 位 sRGB，统一用 RGBA8_SRGB
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case kTexFormatSRGB8_ALPHA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case kTexR10G10B10A2:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        case kTexFormatBGRA32:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        // ---- 半精度/浮点 ----
        case kTexFormatRGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case kTexFormatRG16Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        case kTexFormatRGBA32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        // ---- 深度 ----
        case kTexFormatDepth16:
            return DXGI_FORMAT_D16_UNORM;
        case kTexFormatDepth24:
            // D3D12 无“纯 24 位深度”，用 32 位浮点替代（精度更高，语义兼容）
            return DXGI_FORMAT_D32_FLOAT;
        case kTexFormatDepth32Float:
            return DXGI_FORMAT_D32_FLOAT;
        case kTexFormatDepth16Stencil8:
            // D3D12 不提供 D16S8，退化为 D24S8（只在 shader 采样深度时精度略有差异）
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case kTexFormatDepth24Stencil8:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case kTexFormatDepth32FloatStencil8:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        // ---- BC / DXT ----
        case kTexFormatDXT1_RGB:
            return DXGI_FORMAT_BC1_UNORM;
        case kTexFormatDXT1_SRGB:
            return DXGI_FORMAT_BC1_UNORM_SRGB;
        case kTexFormatDXT3_RGB:
            return DXGI_FORMAT_BC2_UNORM;
        case kTexFormatDXT3_SRGB:
            return DXGI_FORMAT_BC2_UNORM_SRGB;
        case kTexFormatDXT5_RGB:
            return DXGI_FORMAT_BC3_UNORM;
        case kTexFormatDXT5_SRGB:
            return DXGI_FORMAT_BC3_UNORM_SRGB;
        case kTexFormatBC6H_UFLOAT:
            return DXGI_FORMAT_BC6H_UF16;
        case kTexFormatBC6H_SFLOAT:
            return DXGI_FORMAT_BC6H_SF16;
        case kTexFormatBC7_RGB:
            return DXGI_FORMAT_BC7_UNORM;
        case kTexFormatBC7_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        // ---- 32 位标量 ----
        case kTexFormatR32Float:
            return DXGI_FORMAT_R32_FLOAT;
        case kTexFormatR32Uint:
            return DXGI_FORMAT_R32_UINT;
        case kTexFormatR32Sint:
            return DXGI_FORMAT_R32_SINT;
        case kTexFormatRG32Uint:
            return DXGI_FORMAT_R32G32_UINT;
        case kTexFormatRG32Sint:
            return DXGI_FORMAT_R32G32_SINT;
        case kTexFormatRG32Float:
            return DXGI_FORMAT_R32G32_FLOAT;

        // ---- 8 位整型 ----
        case kTexFormatR8Uint:
            return DXGI_FORMAT_R8_UINT;
        case kTexFormatRG8Uint:
            return DXGI_FORMAT_R8G8_UINT;
        case kTexFormatRGBA8Uint:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case kTexFormatR8Sint:
            return DXGI_FORMAT_R8_SINT;
        case kTexFormatRG8Sint:
            return DXGI_FORMAT_R8G8_SINT;
        case kTexFormatRGBA8Sint:
            return DXGI_FORMAT_R8G8B8A8_SINT;

        // ---- 16 位整型 ----
        case kTexFormatR16Uint:
            return DXGI_FORMAT_R16_UINT;
        case kTexFormatRG16Uint:
            return DXGI_FORMAT_R16G16_UINT;
        case kTexFormatRGBA16Uint:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case kTexFormatR16Sint:
            return DXGI_FORMAT_R16_SINT;
        case kTexFormatRG16Sint:
            return DXGI_FORMAT_R16G16_SINT;
        case kTexFormatRGBA16Sint:
            return DXGI_FORMAT_R16G16B16A16_SINT;

        default:
            break;
    }

    // ASTC / PVRTC / ETC / ATC 在桌面 D3D12 上没有对应格式。
    // 正常资产管线在 Windows 上只会产出 BC 系列，走到这里说明输入资产有问题。
    if (IsAnyCompressedTextureFormat(texFormat))
    {
        LOG_ERROR("[DX12] ConvertTextureFormat: compressed format %u (%s) is not supported by D3D12",
                  (unsigned)texFormat, GetCompressionTypeString(texFormat));
    }
    else
    {
        LOG_ERROR("[DX12] ConvertTextureFormat: unsupported TextureFormat %u", (unsigned)texFormat);
    }
    return DXGI_FORMAT_UNKNOWN;
}

bool DX12Util::IsDepthStencilFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;
        default:
            return false;
    }
}

bool DX12Util::IsSRGBFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;
        default:
            return false;
    }
}

DXGI_FORMAT DX12Util::GetTypelessFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_D16_UNORM:            return DXGI_FORMAT_R16_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:            return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:    return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32G8X24_TYPELESS;
        default:                               return format;
    }
}

DXGI_FORMAT DX12Util::GetDepthSRVFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_D16_UNORM:            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D32_FLOAT:            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:                               return format;
    }
}

DXGI_FORMAT DX12Util::ConvertVertexFormat(VertexFormat format)
{
    switch (format)
    {
        case VertexFormatFloat:      return DXGI_FORMAT_R32_FLOAT;
        case VertexFormatFloat2:     return DXGI_FORMAT_R32G32_FLOAT;
        case VertexFormatFloat3:     return DXGI_FORMAT_R32G32B32_FLOAT;
        case VertexFormatFloat4:     return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case VertexFormatInt:        return DXGI_FORMAT_R32_SINT;
        case VertexFormatInt2:       return DXGI_FORMAT_R32G32_SINT;
        case VertexFormatInt3:       return DXGI_FORMAT_R32G32B32_SINT;
        case VertexFormatInt4:       return DXGI_FORMAT_R32G32B32A32_SINT;

        case VertexFormatUInt:       return DXGI_FORMAT_R32_UINT;
        case VertexFormatUInt2:      return DXGI_FORMAT_R32G32_UINT;
        case VertexFormatUInt3:      return DXGI_FORMAT_R32G32B32_UINT;
        case VertexFormatUInt4:      return DXGI_FORMAT_R32G32B32A32_UINT;

        case VertexFormatHalfFloat:  return DXGI_FORMAT_R16_FLOAT;
        case VertexFormatHalfFloat2: return DXGI_FORMAT_R16G16_FLOAT;
        // DXGI 不提供 3 分量 16 位浮点，扩展为 RGBA16_FLOAT（与 R8G8B8 的处理一致）
        case VertexFormatHalfFloat3: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case VertexFormatHalfFloat4: return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case VertexFormatUChar:      return DXGI_FORMAT_R8_UINT;
        case VertexFormatUChar2:     return DXGI_FORMAT_R8G8_UINT;
        case VertexFormatUChar3:     return DXGI_FORMAT_R8G8B8A8_UINT;  // DXGI 无 R8G8B8_UINT，扩展为 4 通道
        case VertexFormatUChar4:     return DXGI_FORMAT_R8G8B8A8_UINT;

        case VertexFormatChar:       return DXGI_FORMAT_R8_SINT;
        case VertexFormatChar2:      return DXGI_FORMAT_R8G8_SINT;
        case VertexFormatChar3:      return DXGI_FORMAT_R8G8B8A8_SINT;
        case VertexFormatChar4:      return DXGI_FORMAT_R8G8B8A8_SINT;

        case VertexFormatUShort:     return DXGI_FORMAT_R16_UINT;
        case VertexFormatUShort2:    return DXGI_FORMAT_R16G16_UINT;
        case VertexFormatUShort3:    return DXGI_FORMAT_R16G16B16A16_UINT;
        case VertexFormatUShort4:    return DXGI_FORMAT_R16G16B16A16_UINT;

        case VertexFormatShort:      return DXGI_FORMAT_R16_SINT;
        case VertexFormatShort2:     return DXGI_FORMAT_R16G16_SINT;
        case VertexFormatShort3:     return DXGI_FORMAT_R16G16B16A16_SINT;
        case VertexFormatShort4:     return DXGI_FORMAT_R16G16B16A16_SINT;

        case VertexFormatUCharNorm:  return DXGI_FORMAT_R8_UNORM;
        case VertexFormatUChar2Norm: return DXGI_FORMAT_R8G8_UNORM;
        case VertexFormatUChar3Norm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case VertexFormatUChar4Norm: return DXGI_FORMAT_R8G8B8A8_UNORM;

        case VertexFormatCharNorm:   return DXGI_FORMAT_R8_SNORM;
        case VertexFormatChar2Norm:  return DXGI_FORMAT_R8G8_SNORM;
        case VertexFormatChar3Norm:  return DXGI_FORMAT_R8G8B8A8_SNORM;
        case VertexFormatChar4Norm:  return DXGI_FORMAT_R8G8B8A8_SNORM;

        default:
            LOG_ERROR("[DX12] ConvertVertexFormat: unsupported VertexFormat %d", (int)format);
            return DXGI_FORMAT_UNKNOWN;
    }
}

uint32_t DX12Util::GetVertexFormatSize(VertexFormat format)
{
    switch (format)
    {
        case VertexFormatFloat:      return 4;
        case VertexFormatFloat2:     return 8;
        case VertexFormatFloat3:     return 12;
        case VertexFormatFloat4:     return 16;

        case VertexFormatInt:
        case VertexFormatUInt:       return 4;
        case VertexFormatInt2:
        case VertexFormatUInt2:      return 8;
        case VertexFormatInt3:
        case VertexFormatUInt3:      return 12;
        case VertexFormatInt4:
        case VertexFormatUInt4:      return 16;

        case VertexFormatHalfFloat:  return 2;
        case VertexFormatHalfFloat2: return 4;
        case VertexFormatHalfFloat3: return 6;
        case VertexFormatHalfFloat4: return 8;

        case VertexFormatUChar:
        case VertexFormatUCharNorm:
        case VertexFormatChar:
        case VertexFormatCharNorm:   return 1;
        case VertexFormatUChar2:
        case VertexFormatUChar2Norm:
        case VertexFormatChar2:
        case VertexFormatChar2Norm:  return 2;
        case VertexFormatUChar3:
        case VertexFormatUChar3Norm:
        case VertexFormatChar3:
        case VertexFormatChar3Norm:  return 3;
        case VertexFormatUChar4:
        case VertexFormatUChar4Norm:
        case VertexFormatChar4:
        case VertexFormatChar4Norm:  return 4;

        case VertexFormatUShort:
        case VertexFormatShort:      return 2;
        case VertexFormatUShort2:
        case VertexFormatShort2:     return 4;
        case VertexFormatUShort3:
        case VertexFormatShort3:     return 6;
        case VertexFormatUShort4:
        case VertexFormatShort4:     return 8;

        default:                     return 0;
    }
}

D3D12_RESOURCE_FLAGS DX12Util::ConvertTextureUsage(ID3D12Device* device, TextureUsage usage,
                                                   DXGI_FORMAT format)
{
    // 先采纳调用方显式声明的用途
    D3D12_RESOURCE_FLAGS flags = ConvertTextureUsage(usage, format);

    if (device == nullptr || format == DXGI_FORMAT_UNKNOWN)
    {
        return flags;
    }

    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};
    formatSupport.Format = format;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport,
                                           sizeof(formatSupport))))
    {
        return flags;
    }

    const bool isDepthStencil = IsDepthStencilFormat(format);
    const bool isSRGB         = IsSRGBFormat(format);

    if (isDepthStencil)
    {
        // 深度模板格式不能同时是渲染目标，两者互斥
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        flags &= ~D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        return flags;
    }

    if ((formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }

    // sRGB 格式不允许作为 typed UAV（D3D12 硬性限制），与 Vulkan 后端的判断一致
    if (!isSRGB &&
        (formatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE) != 0)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return flags;
}

D3D12_RESOURCE_FLAGS DX12Util::ConvertTextureUsage(TextureUsage usage, DXGI_FORMAT format)
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if ((usage & TextureUsage::TextureUsageShaderWrite) != TextureUsage::TextureUsageUnknown)
    {
        // sRGB 资源不能作为 UAV（D3D12 硬性限制），与 Vulkan 后端的处理一致：
        // 在这种情况下静默降级为普通纹理，避免创建失败。
        if (IsSRGBFormat(format))
        {
            LOG_WARN("[DX12] TextureUsageShaderWrite requested on an sRGB format; "
                     "UAV flag dropped (D3D12 does not allow sRGB UAVs).");
        }
        else
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
    }

    if ((usage & TextureUsage::TextureUsageRenderTarget) != TextureUsage::TextureUsageUnknown)
    {
        if (IsDepthStencilFormat(format))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        else
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
    }

    return flags;
}

DXGI_FORMAT DX12Util::ConvertIndexType(IndexType type)
{
    return (type == IndexType_UInt) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
}

// ============================================================================
// 渲染状态
// ============================================================================

D3D12_COMPARISON_FUNC DX12Util::ConvertCompareFunction(CompareFunction function)
{
    switch (function)
    {
        case CompareFunctionNever:              return D3D12_COMPARISON_FUNC_NEVER;
        case CompareFunctionLess:               return D3D12_COMPARISON_FUNC_LESS;
        case CompareFunctionEqual:              return D3D12_COMPARISON_FUNC_EQUAL;
        case CompareFunctionLessThanOrEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case CompareFunctionGreater:            return D3D12_COMPARISON_FUNC_GREATER;
        case CompareFunctionNotEqual:           return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case CompareFunctionGreaterThanOrEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case CompareFunctionAlways:
        default:                                return D3D12_COMPARISON_FUNC_ALWAYS;
    }
}

D3D12_STENCIL_OP DX12Util::ConvertStencilOperation(StencilOperation operation)
{
    switch (operation)
    {
        case StencilOperationZero:           return D3D12_STENCIL_OP_ZERO;
        case StencilOperationReplace:        return D3D12_STENCIL_OP_REPLACE;
        case StencilOperationIncrementClamp: return D3D12_STENCIL_OP_INCR_SAT;
        case StencilOperationDecrementClamp: return D3D12_STENCIL_OP_DECR_SAT;
        case StencilOperationInvert:         return D3D12_STENCIL_OP_INVERT;
        case StencilOperationIncrementWrap:  return D3D12_STENCIL_OP_INCR;
        case StencilOperationDecrementWrap:  return D3D12_STENCIL_OP_DECR;
        case StencilOperationKeep:
        default:                             return D3D12_STENCIL_OP_KEEP;
    }
}

D3D12_BLEND DX12Util::ConvertBlendFactor(BlendFactor factor)
{
    // 注意：引擎枚举与 D3D12 枚举的排列顺序不同（D3D12 把 DEST_ALPHA 排在
    // DEST_COLOR 之前），必须显式映射，不能用加减偏移。
    switch (factor)
    {
        case BlendFactorZero:                     return D3D12_BLEND_ZERO;
        case BlendFactorOne:                      return D3D12_BLEND_ONE;
        case BlendFactorSourceColor:              return D3D12_BLEND_SRC_COLOR;
        case BlendFactorOneMinusSourceColor:      return D3D12_BLEND_INV_SRC_COLOR;
        case BlendFactorSourceAlpha:              return D3D12_BLEND_SRC_ALPHA;
        case BlendFactorOneMinusSourceAlpha:      return D3D12_BLEND_INV_SRC_ALPHA;
        case BlendFactorDestinationColor:         return D3D12_BLEND_DEST_COLOR;
        case BlendFactorOneMinusDestinationColor: return D3D12_BLEND_INV_DEST_COLOR;
        case BlendFactorDestinationAlpha:         return D3D12_BLEND_DEST_ALPHA;
        case BlendFactorOneMinusDestinationAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
        case BlendFactorSourceAlphaSaturated:     return D3D12_BLEND_SRC_ALPHA_SAT;
        case BlendFactorBlendColor:               return D3D12_BLEND_BLEND_FACTOR;
        case BlendFactorOneMinusBlendColor:       return D3D12_BLEND_INV_BLEND_FACTOR;
        case BlendFactorBlendAlpha:               return D3D12_BLEND_BLEND_FACTOR;
        case BlendFactorOneMinusBlendAlpha:       return D3D12_BLEND_INV_BLEND_FACTOR;
        default:                                  return D3D12_BLEND_ONE;
    }
}

D3D12_BLEND_OP DX12Util::ConvertBlendEquation(BlendEquation equation)
{
    switch (equation)
    {
        case BlendEquationAdd:             return D3D12_BLEND_OP_ADD;
        case BlendEquationSubtract:        return D3D12_BLEND_OP_SUBTRACT;
        case BlendEquationReverseSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
        case BlendEquationMinimum:         return D3D12_BLEND_OP_MIN;
        case BlendEquationMaximum:         return D3D12_BLEND_OP_MAX;
        default:                           return D3D12_BLEND_OP_ADD;
    }
}

D3D12_CULL_MODE DX12Util::ConvertCullMode(CullMode mode)
{
    switch (mode)
    {
        case CullModeFront: return D3D12_CULL_MODE_FRONT;
        case CullModeBack:  return D3D12_CULL_MODE_BACK;
        case CullModeNone:
        default:            return D3D12_CULL_MODE_NONE;
    }
}

D3D12_FILL_MODE DX12Util::ConvertFillMode(FillMode mode)
{
    return (mode == FillModeWireframe) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}

D3D12_PRIMITIVE_TOPOLOGY DX12Util::ConvertPrimitiveMode(PrimitiveMode mode)
{
    switch (mode)
    {
        case PrimitiveMode_POINTS:         return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveMode_LINES:          return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveMode_LINE_STRIP:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveMode_TRIANGLES:      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveMode_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:                           return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE DX12Util::ConvertPrimitiveTopologyType(PrimitiveMode mode)
{
    switch (mode)
    {
        case PrimitiveMode_POINTS:         return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveMode_LINES:
        case PrimitiveMode_LINE_STRIP:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveMode_TRIANGLES:
        case PrimitiveMode_TRIANGLE_STRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        default:                           return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

UINT8 DX12Util::ConvertColorWriteMask(ColorWriteMask mask)
{
    // 引擎 ColorWriteMask 的位定义与 D3D12_COLOR_WRITE_ENABLE_* 完全一致
    // （Red=bit3, Green=bit2, Blue=bit1, Alpha=bit0），直接透传。
    return (UINT8)((uint32_t)mask & 0xF);
}

// ============================================================================
// 采样器
// ============================================================================

D3D12_TEXTURE_ADDRESS_MODE DX12Util::ConvertSamplerWrapMode(SamplerWrapMode mode)
{
    switch (mode)
    {
        case CLAMP_TO_EDGE:    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case REPEAT:           return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case MIRRORED_REPEAT:  return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        default:               return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    }
}

D3D12_FILTER DX12Util::ConvertSamplerFilter(const SamplerDesc& des)
{
    const bool minLinear = (des.filterMin == MIN_LINEAR);
    const bool magLinear = (des.filterMag == MAG_LINEAR);
    const bool mipLinear = (des.filterMip == MIN_LINEAR_MIPMAP_LINEAR ||
                            des.filterMip == MIN_LINEAR_MIPMAP_NEAREST);
    const bool mipNone   = (des.filterMip == MIN_NEAREST_MIPMAP_NEAREST ||
                            des.filterMip == MIN_LINEAR_MIPMAP_NEAREST);

    // ── 刻意不映射 des.anisotropyLog2 ──
    // Vulkan 后端固定 anisotropyEnable = VK_FALSE / maxAnisotropy = 1
    // （见 VKTextureSampler::GenerateSamplerInfo），即**完全忽略** SamplerDesc 里的
    // 各向异性设置。D3D12 若按 anisotropyLog2 生成 D3D12_FILTER_ANISOTROPIC，
    // 同一个 SamplerDesc 在两个后端就会得到不同的采样结果：斜视表面上的贴图一个
    // 被各向异性锐化、另一个保持三线性模糊。实测地形 demo（材质 anisotropyLog2 = 2）
    // 仅此一项就造成 0.33% 的像素差异，且因为是跨 API 采样，无法再收敛。
    // 因此这里与 Vulkan 保持一致：一律使用各向同性（等距）过滤。
    // 若将来要让两个后端都启用各向异性，必须同时改 Vulkan 侧，并接受两家驱动的
    // 各向异性抽头（tap）模式不同、无法逐像素对齐的事实。

    // D3D12 的 FILTER 掩码组合：
    //   MIN/MAG/MIP 各占 1 bit（0 = POINT, 1 = LINEAR）
    D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    if (minLinear && magLinear && mipLinear)
    {
        filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    }
    else if (minLinear && magLinear && mipNone)
    {
        filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    }
    else if (minLinear && !magLinear && mipLinear)
    {
        filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    }
    else if (!minLinear && magLinear && mipLinear)
    {
        filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    }
    else if (!minLinear && !magLinear && mipLinear)
    {
        filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    }
    else if (minLinear && !magLinear && mipNone)
    {
        filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    }
    else if (!minLinear && magLinear && mipNone)
    {
        filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    }
    else
    {
        filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    }

    // 比较采样器（阴影贴图 PCF）需要叠加 COMPARISON 标志
    if (des.compareMode == COMPARE_TO_TEXTURE)
    {
        if (filter == D3D12_FILTER_ANISOTROPIC)
        {
            filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
        }
        else
        {
            switch (filter)
            {
                case D3D12_FILTER_MIN_MAG_MIP_POINT:          filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT; break;
                case D3D12_FILTER_MIN_MAG_MIP_LINEAR:         filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; break;
                case D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT:   filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; break;
                case D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR: filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
                case D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR:   filter = D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR; break;
                case D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR:   filter = D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR; break;
                case D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT:   filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT; break;
                case D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: filter = D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
                default: break;
            }
        }
    }

    return filter;
}

void DX12Util::FillSamplerDesc(const SamplerDesc& des, D3D12_SAMPLER_DESC& outDesc)
{
    memset(&outDesc, 0, sizeof(outDesc));
    outDesc.Filter         = ConvertSamplerFilter(des);
    outDesc.AddressU       = ConvertSamplerWrapMode(des.wrapS);
    outDesc.AddressV       = ConvertSamplerWrapMode(des.wrapT);
    outDesc.AddressW       = ConvertSamplerWrapMode(des.wrapR);
    outDesc.MipLODBias     = 0.0f;
    // 与 Vulkan 后端保持一致：各向异性被忽略（见 ConvertSamplerFilter 的说明），
    // 因此 MaxAnisotropy 恒为 1；非各向异性 filter 下 D3D 本来就忽略该字段。
    outDesc.MaxAnisotropy  = 1u;
    (void)des.anisotropyLog2;
    outDesc.ComparisonFunc = ConvertCompareFunction(des.compareFunc);
    outDesc.BorderColor[0] = 0.0f;
    outDesc.BorderColor[1] = 0.0f;
    outDesc.BorderColor[2] = 0.0f;
    outDesc.BorderColor[3] = 0.0f;
    // Vulkan 后端在 SamplerDesc 未显式指定 LOD 时默认 minLod=0/maxLod=0，
    // 会把 mip 限制在第 0 层。这里保持同样语义（maxLod=0 时按 0 处理），
    // 由上层在创建可 mip 采样的采样器时显式给出 maxLod。
    outDesc.MinLOD         = (float)des.minLod;
    outDesc.MaxLOD         = (float)des.maxLod;
}

// ============================================================================
// 资源状态
// ============================================================================

D3D12_RESOURCE_STATES DX12Util::ConvertTextureAccessType(ResourceAccessType accessType, DXGI_FORMAT format)
{
    const uint32_t v = (uint32_t)accessType;

    // 写状态优先（同一时刻不可能同时是 RT 和 SRV，这里按“主要角色”判定）
    if (v & (uint32_t)ResourceAccessType::ColorAttachment)
    {
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (v & (uint32_t)ResourceAccessType::DepthStencilAttachment)
    {
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if (v & (uint32_t)ResourceAccessType::DepthStencilReadOnly)
    {
        return D3D12_RESOURCE_STATE_DEPTH_READ;
    }
    if (v & (uint32_t)ResourceAccessType::ComputeShaderWrite)
    {
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if (v & (uint32_t)ResourceAccessType::TransferDst)
    {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (v & ((uint32_t)ResourceAccessType::ComputeShaderRead | (uint32_t)ResourceAccessType::ShaderRead))
    {
        // 用 ALL_SHADER_RESOURCE（= PIXEL | NON_PIXEL）而不是只给 PIXEL，
        // 因为同一个纹理可能被顶点/网格/计算阶段采样。牺牲一点状态精度换正确性。
        return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    }
    if (v & (uint32_t)ResourceAccessType::IndirectCommandRead)
    {
        return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }
    if (v & (uint32_t)ResourceAccessType::TransferSrc)
    {
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    }

    return D3D12_RESOURCE_STATE_COMMON;
}

D3D12_RESOURCE_STATES DX12Util::ConvertBufferAccessType(ResourceAccessType accessType)
{
    const uint32_t v = (uint32_t)accessType;

    if (v & (uint32_t)ResourceAccessType::ComputeShaderWrite)
    {
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if (v & (uint32_t)ResourceAccessType::TransferDst)
    {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (v & (uint32_t)ResourceAccessType::TransferSrc)
    {
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (v & (uint32_t)ResourceAccessType::IndirectCommandRead)
    {
        return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }
    if (v & ((uint32_t)ResourceAccessType::ComputeShaderRead | (uint32_t)ResourceAccessType::ShaderRead))
    {
        return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    }

    return D3D12_RESOURCE_STATE_COMMON;
}

// ============================================================================
// 辅助
// ============================================================================

void DX12Util::GetMipDimensions(uint32_t width, uint32_t height, uint32_t mip,
                                uint32_t& outWidth, uint32_t& outHeight)
{
    outWidth  = (width  >> mip) ? (width  >> mip) : 1u;
    outHeight = (height >> mip) ? (height >> mip) : 1u;
}

uint32_t DX12Util::GetFormatBitsPerPixel(TextureFormat format)
{
    switch (ConvertTextureFormat(format))
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:   return 128;
        case DXGI_FORMAT_R32G32B32_FLOAT:     return 96;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:         return 64;
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:         return 32;
        case DXGI_FORMAT_R8G8_UNORM:          return 16;
        case DXGI_FORMAT_R8_UNORM:            return 8;
        default:                              return 32;
    }
}

uint64_t DX12Util::GetMipRowBytes(TextureFormat format, uint32_t mipWidth)
{
    TextureBlockInfo blockInfo = GetCompressedTextureBlockInfo(format);
    if (blockInfo.bytesPerBlock > 0)
    {
        // 块压缩：行字节 = 该行的块数 × 每块字节数
        const uint32_t blockWidth = (blockInfo.blockWidth > 0) ? blockInfo.blockWidth : 4u;
        const uint32_t blocksPerRow = (mipWidth + blockWidth - 1) / blockWidth;
        return (uint64_t)blocksPerRow * blockInfo.bytesPerBlock;
    }

    // Use DXGI sizing for formats rejected by the generic texture helper.
    LOG_WARN("[DX12] GetMipRowBytes fallback for format=%d width=%u", (int)format, mipWidth);
    fflush(stderr);

    const uint32_t bitsPerPixel = DX12Util::GetFormatBitsPerPixel(format);
    const uint32_t bytesPerPixel = (bitsPerPixel + 7u) / 8u;
    return (uint64_t)bytesPerPixel * mipWidth;
}

NAMESPACE_RENDERCORE_END
