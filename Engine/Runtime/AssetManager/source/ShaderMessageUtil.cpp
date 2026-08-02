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

ByteVectorPtr ShaderMessageUtil::EncodeShaderMessage(const ShaderMessage& msg)
{
    // 将 repeated 字段挂到回调 arg 上
    ShaderMessage encMsg = msg;

    std::vector<UniformBufferLayoutMessage> layouts;
    std::vector<PushConstantMessage> pushConsts;
    std::vector<ShaderResourceMessage> resources;
    std::vector<VertexInputMessage> vertexInputs;

    // 为每个 uniformBuffer 设置它的 members 回调（因为 UniformBufferLayoutMessage
    // 内部有 repeated UniformMemberMessage，nanopb 需要回调）
    // 这里简化处理：nanopb 对嵌套 repeated submessage 的限制较大，
    // 实际上我们是平铺序列化，所有回调 arg 用动态 vector。
    // 注意：当前 proto 设计的 uniformBuffers 直接包含 repeated UniformMemberMessage，
    // 这在 nanopb 中需要逐层处理回调。为保持一致性，我们在调用前分配好数据。

    encMsg.uniformBuffers.funcs.encode = nanopb_encode_uniform_buffer_layout;
    encMsg.uniformBuffers.arg = &layouts;

    encMsg.pushConstants.funcs.encode = nanopb_encode_push_constant;
    encMsg.pushConstants.arg = &pushConsts;

    encMsg.resources.funcs.encode = nanopb_encode_shader_resource;
    encMsg.resources.arg = &resources;

    encMsg.vertexInputs.funcs.encode = nanopb_encode_vertex_input;
    encMsg.vertexInputs.arg = &vertexInputs;

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

    std::vector<UniformBufferLayoutMessage> layouts;
    std::vector<PushConstantMessage> pushConsts;
    std::vector<ShaderResourceMessage> resources;
    std::vector<VertexInputMessage> vertexInputs;

    msg.uniformBuffers.funcs.decode = nanopb_decode_uniform_buffer_layout;
    msg.uniformBuffers.arg = &layouts;

    msg.pushConstants.funcs.decode = nanopb_decode_push_constant;
    msg.pushConstants.arg = &pushConsts;

    msg.resources.funcs.decode = nanopb_decode_shader_resource;
    msg.resources.arg = &resources;

    msg.vertexInputs.funcs.decode = nanopb_decode_vertex_input;
    msg.vertexInputs.arg = &vertexInputs;

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

    pb_release(ShaderMessage_fields, &msg);
}

NS_ASSETMANAGER_END
