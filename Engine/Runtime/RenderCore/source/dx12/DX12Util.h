//
//  DX12Util.h
//  rendercore
//
//  D3D12 后端的纯函数工具集：引擎枚举 → D3D12 枚举的映射。
//  与 VulkanBufferUtil 中的同名转换一一对应，保证两个后端语义一致。
//

#ifndef GNX_ENGINE_DX12_UTIL_INCLUDE_JHDSG
#define GNX_ENGINE_DX12_UTIL_INCLUDE_JHDSG

#include "DX12RenderDefine.h"
#include "RenderDescriptor.h"
#include "TextureFormat.h"

NAMESPACE_RENDERCORE_BEGIN

class DX12Util
{
public:
    // ==================== 格式 ====================

    /// 引擎纹理格式 → DXGI 格式（返回 DXGI_FORMAT_UNKNOWN 表示不支持）
    static DXGI_FORMAT ConvertTextureFormat(TextureFormat texFormat);

    /// 该 DXGI 格式是否为深度/模板格式
    static bool IsDepthStencilFormat(DXGI_FORMAT format);

    /**
     * @brief 该深度/模板格式是否含有 stencil 平面
     *
     * RenderPass 需要分别为 depth / stencil 平面声明 beginning/ending access；
     * 对不含 stencil 平面的格式（如 D32_FLOAT）必须声明 NO_ACCESS，否则运行时
     * 会认为模板平面被访问而校验失败。
     */
    static bool HasStencilPlane(DXGI_FORMAT format);

    /// 该 DXGI 格式是否带 sRGB 编码
    static bool IsSRGBFormat(DXGI_FORMAT format);

    /// 深度格式创建资源时使用的 typeless 格式（D3D12 要求 depth 资源可被 SRV 采样时须为 typeless）
    static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);

    /// 深度资源作为 SRV 采样时的格式（如 D32_FLOAT → R32_FLOAT）
    static DXGI_FORMAT GetDepthSRVFormat(DXGI_FORMAT format);

    /// 顶点属性格式 → DXGI 格式
    static DXGI_FORMAT ConvertVertexFormat(VertexFormat format);

    /// 顶点属性格式的字节大小
    static uint32_t GetVertexFormatSize(VertexFormat format);

    /// 引擎 TextureUsage + 格式 → D3D12 资源标志
    static D3D12_RESOURCE_FLAGS ConvertTextureUsage(TextureUsage usage, DXGI_FORMAT format);

    /**
     * @brief 非块压缩格式的每像素位宽
     *
     * 用于 GetMipRowBytes 的回退路径。块压缩格式不走这里（由
     * GetCompressedTextureBlockInfo 处理），因此本函数只需覆盖普通与
     * float / 深度 格式；未列出的按 32bpp 处理。
     *
     * 存在的理由：GetMipRowBytes 过去在该回退路径上直接调用
     * GetBytesFromTextureFormat，而后者只接受特定的非压缩格式集合、对
     * float/深度等格式会触发 assert；断言在 demo 宿主中会弹模态框并永久
     * 阻塞进程，表现为"窗口空白、无报错、进程活着"。
     */
    static uint32_t GetFormatBitsPerPixel(TextureFormat format);

    /**
     * @brief 在「已声明用途」的基础上，追加该 DXGI 格式实际支持的能力
     *
     * 对齐 Vulkan 后端的既有策略：VKTextureBase 会查询
     * vkGetPhysicalDeviceFormatProperties2 的 formatFeatures，并据此打开
     * STORAGE_IMAGE / COLOR_ATTACHMENT / DEPTH_STENCIL_ATTACHMENT / TRANSFER_* 等
     * 所有可用用途（VKTextureBase.cpp 的 imageCreateInfoCopy 那段）。
     *
     * 引擎的部分调用方会少声明用途（例如把纹理创建为「仅渲染目标」却在 shader 里
     * 当 UAV 写），在 Vulkan 下因为「能开就开」不会出问题；D3D12 若严格按声明创建，
     * 就会在 CreateUnorderedAccessView / ResourceBarrier 处报错：
     *   id=340 "UAV cannot be created of a Resource that did not specify
     *           D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS"
     *   id=524 "RESOURCE_STATE_UNORDERED_ACCESS has bits that mismatch
     *           D3D12_RESOURCE_FLAGS"
     * 因此这里采用与 Vulkan 后端一致的宽松策略。
     *
     * @param device 用于查询格式能力；为 nullptr 时退化为只使用已声明用途
     */
    static D3D12_RESOURCE_FLAGS ConvertTextureUsage(ID3D12Device* device, TextureUsage usage,
                                                    DXGI_FORMAT format);

    /// 引擎索引类型 → DXGI 格式
    static DXGI_FORMAT ConvertIndexType(IndexType type);

    // ==================== 渲染状态 ====================

    static D3D12_COMPARISON_FUNC ConvertCompareFunction(CompareFunction function);
    static D3D12_STENCIL_OP      ConvertStencilOperation(StencilOperation operation);
    static D3D12_BLEND           ConvertBlendFactor(BlendFactor factor);
    static D3D12_BLEND_OP        ConvertBlendEquation(BlendEquation equation);
    static D3D12_CULL_MODE       ConvertCullMode(CullMode mode);
    static D3D12_FILL_MODE       ConvertFillMode(FillMode mode);
    static D3D12_PRIMITIVE_TOPOLOGY ConvertPrimitiveMode(PrimitiveMode mode);
    static D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertPrimitiveTopologyType(PrimitiveMode mode);

    /// 引擎 ColorWriteMask → D3D12 写入通道掩码
    static UINT8 ConvertColorWriteMask(ColorWriteMask mask);

    // ==================== 采样器 ====================

    static D3D12_FILTER ConvertSamplerFilter(const SamplerDesc& des);
    static D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerWrapMode(SamplerWrapMode mode);

    /// 完整填充 D3D12_SAMPLER_DESC
    static void FillSamplerDesc(const SamplerDesc& des, D3D12_SAMPLER_DESC& outDesc);

    // ==================== 资源状态（barrier） ====================

    /// 引擎访问类型 → D3D12 资源状态
    static D3D12_RESOURCE_STATES ConvertTextureAccessType(ResourceAccessType accessType, DXGI_FORMAT format);

    /// 引擎访问类型 → buffer 的 D3D12 资源状态
    static D3D12_RESOURCE_STATES ConvertBufferAccessType(ResourceAccessType accessType);

    // ==================== 辅助 ====================

    /// 对齐到指定边界
    static uint64_t AlignUp(uint64_t value, uint64_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    /// 计算某个 mip 层级的尺寸
    static void GetMipDimensions(uint32_t width, uint32_t height, uint32_t mip,
                                 uint32_t& outWidth, uint32_t& outHeight);

    /// 计算某个 mip 层级的行字节数（已按块压缩与 256 字节行对齐规则处理）
    static uint64_t GetMipRowBytes(TextureFormat format, uint32_t mipWidth);
};

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_DX12_UTIL_INCLUDE_JHDSG */
