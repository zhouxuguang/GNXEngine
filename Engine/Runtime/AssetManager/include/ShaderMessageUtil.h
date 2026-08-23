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

private:
    ShaderMessageUtil();
    ~ShaderMessageUtil();
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_SHADERMESSAGE_UTIL_INCLUDE_SDGMJFKJ
