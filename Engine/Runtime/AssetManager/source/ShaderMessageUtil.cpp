#include "ShaderMessageUtil.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "ShaderMessage.pb.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "PBUtils.h"

#include <cstring>
#include <cstdlib>

NS_ASSETMANAGER_BEGIN

// ====================================================================
// Encode callbacks - 序列化 repeated submessages
// ====================================================================

static bool nanopb_encode_uniform_member(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    auto* pMembers = (std::vector<UniformMemberMessage>*)*arg;
    if (!pMembers) return true;

    for (auto& m : *pMembers)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;
        if (!pb_encode_submessage(stream, UniformMemberMessage_fields, &m)) return false;
    }
    return true;
}

static bool nanopb_encode_uniform_buffer_layout(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    auto* pLayouts = (std::vector<UniformBufferLayoutMessage>*)*arg;
    if (!pLayouts) return true;

    for (auto& layout : *pLayouts)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;
        if (!pb_encode_submessage(stream, UniformBufferLayoutMessage_fields, &layout)) return false;
    }
    return true;
}

static bool nanopb_encode_push_constant(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    auto* pList = (std::vector<PushConstantMessage>*)*arg;
    if (!pList) return true;

    for (auto& pc : *pList)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;
        if (!pb_encode_submessage(stream, PushConstantMessage_fields, &pc)) return false;
    }
    return true;
}

static bool nanopb_encode_shader_resource(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    auto* pList = (std::vector<ShaderResourceMessage>*)*arg;
    if (!pList) return true;

    for (auto& r : *pList)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;
        if (!pb_encode_submessage(stream, ShaderResourceMessage_fields, &r)) return false;
    }
    return true;
}

static bool nanopb_encode_vertex_input(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
    auto* pList = (std::vector<VertexInputMessage>*)*arg;
    if (!pList) return true;

    for (auto& vi : *pList)
    {
        if (!pb_encode_tag_for_field(stream, field)) return false;
        if (!pb_encode_submessage(stream, VertexInputMessage_fields, &vi)) return false;
    }
    return true;
}

// ====================================================================
// Decode callbacks - 反序列化 repeated submessages
// ====================================================================

static bool nanopb_decode_uniform_member(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
    auto* pMembers = (std::vector<UniformMemberMessage>*)*arg;
    if (!pMembers)
    {
        pMembers = new std::vector<UniformMemberMessage>();
        *arg = pMembers;
    }

    UniformMemberMessage m = UniformMemberMessage_init_default;
    if (!pb_decode(stream, UniformMemberMessage_fields, &m))
        return false;
    pMembers->push_back(m);
    return true;
}

static bool nanopb_decode_uniform_buffer_layout(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
    auto* pLayouts = (std::vector<UniformBufferLayoutMessage>*)*arg;
    if (!pLayouts)
    {
        pLayouts = new std::vector<UniformBufferLayoutMessage>();
        *arg = pLayouts;
    }

    UniformBufferLayoutMessage layout = UniformBufferLayoutMessage_init_default;
    if (!pb_decode(stream, UniformBufferLayoutMessage_fields, &layout))
        return false;
    pLayouts->push_back(layout);
    return true;
}

static bool nanopb_decode_push_constant(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
    auto* pList = (std::vector<PushConstantMessage>*)*arg;
    if (!pList)
    {
        pList = new std::vector<PushConstantMessage>();
        *arg = pList;
    }

    PushConstantMessage pc = PushConstantMessage_init_default;
    if (!pb_decode(stream, PushConstantMessage_fields, &pc))
        return false;
    pList->push_back(pc);
    return true;
}

static bool nanopb_decode_shader_resource(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
    auto* pList = (std::vector<ShaderResourceMessage>*)*arg;
    if (!pList)
    {
        pList = new std::vector<ShaderResourceMessage>();
        *arg = pList;
    }

    ShaderResourceMessage r = ShaderResourceMessage_init_default;
    if (!pb_decode(stream, ShaderResourceMessage_fields, &r))
        return false;
    pList->push_back(r);
    return true;
}

static bool nanopb_decode_vertex_input(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
    auto* pList = (std::vector<VertexInputMessage>*)*arg;
    if (!pList)
    {
        pList = new std::vector<VertexInputMessage>();
        *arg = pList;
    }

    VertexInputMessage vi = VertexInputMessage_init_default;
    if (!pb_decode(stream, VertexInputMessage_fields, &vi))
        return false;
    pList->push_back(vi);
    return true;
}

// ====================================================================
// ShaderMessageUtil 实现
// ====================================================================

ByteVectorPtr ShaderMessageUtil::EncodeShaderMessage(const ShaderMessage& msg,
                                                     const ShaderMessageEncodeData& data)
{
    // 将 repeated 字段挂到回调 arg 上
    ShaderMessage encMsg = msg;

    // 使用调用方填充的数据（避免局部空 vector 覆盖调用方内容）
    encMsg.uniformBuffers.funcs.encode = nanopb_encode_uniform_buffer_layout;
    encMsg.uniformBuffers.arg = const_cast<void*>(static_cast<const void*>(&data.uniformBuffers));

    encMsg.pushConstants.funcs.encode = nanopb_encode_push_constant;
    encMsg.pushConstants.arg = const_cast<void*>(static_cast<const void*>(&data.pushConstants));

    encMsg.resources.funcs.encode = nanopb_encode_shader_resource;
    encMsg.resources.arg = const_cast<void*>(static_cast<const void*>(&data.resources));

    encMsg.vertexInputs.funcs.encode = nanopb_encode_vertex_input;
    encMsg.vertexInputs.arg = const_cast<void*>(static_cast<const void*>(&data.vertexInputs));

    // 计算编码大小
    size_t encodedSize = 0;
    pb_get_encoded_size(&encodedSize, ShaderMessage_fields, &encMsg);

    ByteVectorPtr buffer = std::make_shared<ByteVector>();
    buffer->resize(encodedSize);

    pb_ostream_t encStream = pb_ostream_from_buffer(buffer->data(), buffer->size());
    if (!pb_encode(&encStream, ShaderMessage_fields, &encMsg))
    {
        LOG_INFO("pb encode error in EncodeShaderMessage [%s]\n", PB_GET_ERROR(&encStream));
        buffer->clear();
    }

    return buffer;
}

bool ShaderMessageUtil::DecodeShaderMessage(const uint8_t* pData, uint32_t dataSize, ShaderMessage& msg)
{
    if (!pData || dataSize == 0)
        return false;

    msg = ShaderMessage_init_default;

    // 注意：所有 repeated 回调的 arg 必须设为 nullptr，让 nanopb 回调自分配并挂到 msg 上。
    // 若传入局部 vector 的地址，decode 完成后局部对象销毁，msg 里的 arg 变悬垂指针（崩溃）。
    // 自分配的内存由 ReleaseShaderMessage 释放。

    // compiledShader bytes 解码回调（分配 pb_bytes_array_t）
    msg.compiledShader.funcs.decode = nanopb_decode_gnx_bytes;
    msg.compiledShader.arg = nullptr;

    // entryPoint string 解码回调
    msg.entryPoint.funcs.decode = nanopb_decode_gnx_bytes;   // string 也用 bytes 回调（读入 pb_bytes_array_t）
    msg.entryPoint.arg = nullptr;

    msg.uniformBuffers.funcs.decode = nanopb_decode_uniform_buffer_layout;
    msg.uniformBuffers.arg = nullptr;

    msg.pushConstants.funcs.decode = nanopb_decode_push_constant;
    msg.pushConstants.arg = nullptr;

    msg.resources.funcs.decode = nanopb_decode_shader_resource;
    msg.resources.arg = nullptr;

    msg.vertexInputs.funcs.decode = nanopb_decode_vertex_input;
    msg.vertexInputs.arg = nullptr;

    pb_istream_t decStream = pb_istream_from_buffer(pData, dataSize);
    if (!pb_decode(&decStream, ShaderMessage_fields, &msg))
    {
        LOG_INFO("pb decode error in DecodeShaderMessage %s [%s]\n", __func__, decStream.errmsg);
        return false;
    }

    return true;
}

void ShaderMessageUtil::ReleaseShaderMessage(ShaderMessage& msg)
{
    // 释放动态分配的 vector
    if (msg.uniformBuffers.arg)
    {
        delete (std::vector<UniformBufferLayoutMessage>*)msg.uniformBuffers.arg;
        msg.uniformBuffers.arg = nullptr;
    }
    if (msg.pushConstants.arg)
    {
        delete (std::vector<PushConstantMessage>*)msg.pushConstants.arg;
        msg.pushConstants.arg = nullptr;
    }
    if (msg.resources.arg)
    {
        delete (std::vector<ShaderResourceMessage>*)msg.resources.arg;
        msg.resources.arg = nullptr;
    }
    if (msg.vertexInputs.arg)
    {
        delete (std::vector<VertexInputMessage>*)msg.vertexInputs.arg;
        msg.vertexInputs.arg = nullptr;
    }

    // 释放 bytes 字段
    if (msg.compiledShader.arg)
    {
        free(msg.compiledShader.arg);
        msg.compiledShader.arg = nullptr;
    }
    if (msg.entryPoint.arg)
    {
        free(msg.entryPoint.arg);
        msg.entryPoint.arg = nullptr;
    }

    pb_release(ShaderMessage_fields, &msg);
}

NS_ASSETMANAGER_END
