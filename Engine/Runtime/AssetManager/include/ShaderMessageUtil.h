#ifndef GNX_ENGINE_SHADERMESSAGE_UTIL_INCLUDE_SDGMJFKJ
#define GNX_ENGINE_SHADERMESSAGE_UTIL_INCLUDE_SDGMJFKJ

#include "AssetDefine.h"
#include "ShaderMessage.pb.h"
#include <vector>
#include <cstdint>
#include <string>

NS_ASSETMANAGER_BEGIN

/**
 * ShaderMessage 序列化/反序列化工具
 *
 * Encode:  ShaderMessage 结构体 → protobuf 字节流
 * Decode:  protobuf 字节流     → ShaderMessage 结构体
 */

// 携带 ShaderMessage 中 repeated 反射字段的容器（由调用方填充后传入 Encode）
// nanopb 的回调 arg 需要指向调用方的数据，故不能由 Encode 内部持有局部空 vector
struct ShaderMessageEncodeData
{
    std::vector<UniformBufferLayoutMessage> uniformBuffers;
    std::vector<PushConstantMessage> pushConstants;
    std::vector<ShaderResourceMessage> resources;
    std::vector<VertexInputMessage> vertexInputs;
};

class ShaderMessageUtil
{
public:
    /**
     * 将 ShaderMessage 序列化为 protobuf 字节流
     * @param msg  已填充的 ShaderMessage（标量 + bytes 字段）
     * @param data 反射 repeated 字段数据（uniformBuffers/pushConstants/resources/vertexInputs）
     * @return     序列化后的字节数组，失败返回 nullptr
     */
    static ByteVectorPtr EncodeShaderMessage(const ShaderMessage& msg,
                                             const ShaderMessageEncodeData& data);

    /**
     * 将 protobuf 字节流反序列化为 ShaderMessage
     * @param pData   protobuf 数据指针
     * @param dataSize 数据大小
     * @param msg      输出的 ShaderMessage（调用者需初始化）
     * @return         是否成功
     */
    static bool DecodeShaderMessage(const uint8_t* pData, uint32_t dataSize, ShaderMessage& msg);

    /**
     * 释放 DecodeShaderMessage 中动态分配的内存
     */
    static void ReleaseShaderMessage(ShaderMessage& msg);

    // ==================== 容器（ShaderPackageMessage） ====================

    /**
     * 编码 stages 的辅助条目：一个 stage 的 ShaderMessage + 其反射 repeated 数据。
     * 反射字段的 encode 回调需要指向调用方持有的 vector，故打包成一条。
     * 调用方构造后填入 ShaderPackageMessage.stages（用 set 接口）。
     */
    struct ShaderStageEncodeEntry
    {
        ShaderMessage msg;
        ShaderMessageEncodeData encodeData;
    };

    /**
     * 将 ShaderPackageMessage 序列化为 protobuf 字节流
     * @param pkg 已填充的 ShaderPackageMessage（含 shaderName/format）
     * @param stages std::vector<ShaderStageEncodeEntry>，每个 stage 的 msg + encodeData
     * @return    序列化后的字节数组，失败返回 nullptr
     */
    static ByteVectorPtr EncodeShaderPackage(const ShaderPackageMessage& pkg,
                                             const std::vector<ShaderStageEncodeEntry>& stages);

    /**
     * 将 protobuf 字节流反序列化为 ShaderPackageMessage
     *
     * 注意：stages 是 nested repeated submessage，其内部每个 ShaderMessage 的
     * callback 字段（compiledShader/entryPoint/反射）必须在解码前设置 decode 回调，
     * 由内部的 stages 回调逐 stage 配置（nanopb 对无回调的 callback 字段直接跳过）。
     *
     * @param pData   protobuf 数据指针
     * @param dataSize 数据大小
     * @param pkg      输出的 ShaderPackageMessage（调用者需初始化）
     * @return         是否成功
     */
    static bool DecodeShaderPackage(const uint8_t* pData, uint32_t dataSize, ShaderPackageMessage& pkg);

    /**
     * 释放 DecodeShaderPackage 中动态分配的内存
     * 递归释放每个 stage 的 ShaderMessage（复用 ReleaseShaderMessage）+ stages 容器
     */
    static void ReleaseShaderPackage(ShaderPackageMessage& pkg);

private:
    ShaderMessageUtil();
    ~ShaderMessageUtil();
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_SHADERMESSAGE_UTIL_INCLUDE_SDGMJFKJ
