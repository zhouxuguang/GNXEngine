#include "MeshMessageUtil.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "MeshMessage.pb.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "PBUtils.h"

NS_ASSETPROCESS_BEGIN

bool nanopb_encode_gnx_submeshinfo(pb_ostream_t* stream, const pb_field_t* field, void* const* arg) 
{
	std::vector<SubMeshMessage>* pSubMeshInfos = (std::vector<SubMeshMessage>*)*arg;
	if (!pSubMeshInfos)
	{
		return true;
	}

	for (size_t i = 0; i < pSubMeshInfos->size(); i ++)
	{
		SubMeshMessage& subMeshInfo = (*pSubMeshInfos)[i];

		if (!pb_encode_tag(stream, PB_WT_STRING, field->tag)) 
		{
			return false;
		}

		if (!pb_encode_submessage(stream, SubMeshMessage_fields, &subMeshInfo)) 
		{
			return false;
		}
	}

	return true;
}

bool nanopb_decode_gnx_submeshinfo(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
	std::vector<SubMeshMessage>* pSubMeshInfos = (std::vector<SubMeshMessage>*)*arg;
	if (!pSubMeshInfos)
	{
		pSubMeshInfos = new std::vector<SubMeshMessage>();
		*arg = pSubMeshInfos;
	}

	while (stream->bytes_left > 0)
	{
		SubMeshMessage subMeshInfo = SubMeshMessage_init_default;
		if (!pb_decode(stream, SubMeshMessage_fields, &subMeshInfo))
		{
			return false;
		}
		pSubMeshInfos->push_back(subMeshInfo);
	}

	return true;
}

bool nanopb_encode_gnx_vertex_channel_info(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
	std::vector<VertexChannelMessage>* pVertexChannelInfos = (std::vector<VertexChannelMessage>*)*arg;
	if (!pVertexChannelInfos)
	{
		return true;
	}

	for (size_t i = 0; i < pVertexChannelInfos->size(); i++)
	{
		VertexChannelMessage& vertexChannelInfo = (*pVertexChannelInfos)[i];

		if (!pb_encode_tag_for_field(stream, field))
		{
			return false;
		}

		if (!pb_encode_submessage(stream, VertexChannelMessage_fields, &vertexChannelInfo))
		{
			return false;
		}
	}

	return true;
}

bool nanopb_decode_gnx_vertex_channel_info(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
	std::vector<VertexChannelMessage>* pChannelInfos = (std::vector<VertexChannelMessage>*)*arg;
	if (!pChannelInfos)
	{
		pChannelInfos = new std::vector<VertexChannelMessage>();
		*arg = pChannelInfos;
	}

	while (stream->bytes_left > 0)
	{
		VertexChannelMessage channelInfo = VertexChannelMessage_init_default;
		if (!pb_decode(stream, VertexChannelMessage_fields, &channelInfo))
		{
			return false;
		}

		pChannelInfos->push_back(channelInfo);
	}

	// 这里返回true处理下一个repeated的值
	return true;
}

MeshMessageUtil::MeshMessageUtil()
{
}

MeshMessageUtil::~MeshMessageUtil()
{
}

bool MeshMessageUtil::DecodeMeshMessage(const uint8_t* pData, uint32_t dataSize, Mesh* mesh)
{
	if (!pData || dataSize <= 0 || !mesh)
	{
		return false;
	}

	// 1 先填充结构体，设置编解码回调函数 2 执行解码
	MeshMessage meshMessage = MeshMessage_init_default;
	meshMessage.vertexData.funcs.decode = nanopb_decode_gnx_bytes;
	meshMessage.indiceData.funcs.decode = nanopb_decode_gnx_bytes;
	meshMessage.subMeshInfos.funcs.decode = nanopb_decode_gnx_submeshinfo;
	meshMessage.vertexChannelInfos.funcs.decode = nanopb_decode_gnx_vertex_channel_info;

	// decode data
	pb_istream_t dec_stream = pb_istream_from_buffer(pData, dataSize);
	if (!pb_decode(&dec_stream, MeshMessage_fields, &meshMessage))
	{
		LOG_INFO("pb decode error in %s, error %s\n", __func__, dec_stream.errmsg);
		return false;
	}

	VertexData& vertexData = mesh->GetVertexData();
	vertexData.Resize(meshMessage.vertexCount, meshMessage.vertexSize);

	pb_bytes_array_t* pVertexArray = (pb_bytes_array_t*)meshMessage.vertexData.arg;
	memcpy(vertexData.GetDataPtr(), pVertexArray->bytes, pVertexArray->size);

	pb_bytes_array_t* pIndiceArray = (pb_bytes_array_t*)meshMessage.indiceData.arg;
	mesh->SetIndices((const uint32_t*)pIndiceArray->bytes, pIndiceArray->size / sizeof(uint32_t));

	std::vector<VertexChannelMessage>* pChannelInfos = (std::vector<VertexChannelMessage>*)meshMessage.vertexChannelInfos.arg;

	ChannelInfo* channels = vertexData.GetChannels();
	for (size_t i = 0; i < pChannelInfos->size(); i ++)
	{
		VertexChannel vertexChannel = (*pChannelInfos)[i].vertexChannel;
		channels[vertexChannel].offset = (*pChannelInfos)[i].offset;
		channels[vertexChannel].stride = (*pChannelInfos)[i].stride;
		channels[vertexChannel].format = (VertexFormat)(*pChannelInfos)[i].format;
	}

	std::vector<SubMeshMessage>* pSubMeshInfos = (std::vector<SubMeshMessage>*)meshMessage.subMeshInfos.arg;
	for (size_t i = 0; i < pSubMeshInfos->size(); i ++)
	{
		SubMeshInfo subMeshInfo;
		subMeshInfo.firstIndex = (*pSubMeshInfos)[i].firstIndex;
		subMeshInfo.indexCount = (*pSubMeshInfos)[i].indexCount;
		subMeshInfo.vertexCount = (*pSubMeshInfos)[i].vertexCount;
		subMeshInfo.topology = (PrimitiveMode)(*pSubMeshInfos)[i].topology;
		mesh->AddSubMeshInfo(subMeshInfo);
	}

	return true;
}

ByteVectorPtr MeshMessageUtil::EncodeMeshMessage(const Mesh* mesh)
{
	// 1 先填充结构体的值 2 获得编码后数据大小，分配编码后空间 3 编码
	MeshMessage meshMessage = MeshMessage_init_default;
	meshMessage.indiceType = IndiceType::IndiceType_UnsignedInt;
	meshMessage.indiceCount = mesh->GetIndices().size();
	meshMessage.vertexCount = mesh->GetVertexCount();
	meshMessage.vertexSize = mesh->GetVertexSize();

	const VertexData& vertexData = mesh->GetVertexData();

	// 顶点数据
	pb_bytes_array_t* pDataBytes = (pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(vertexData.GetDataSize()));
	pDataBytes->size = vertexData.GetDataSize();
	meshMessage.vertexData.arg = pDataBytes;
	meshMessage.vertexData.funcs.encode = nanopb_encode_gnx_bytes;
	memcpy(pDataBytes->bytes, vertexData.GetDataPtr(), pDataBytes->size);

	// 索引数据
	pb_bytes_array_t* pIndiceBytes = (pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(mesh->GetIndices().size() * sizeof(uint32_t)));
	pIndiceBytes->size = mesh->GetIndices().size() * sizeof(uint32_t);
	meshMessage.indiceData.arg = pIndiceBytes;
	meshMessage.indiceData.funcs.encode = nanopb_encode_gnx_bytes;
	memcpy(pIndiceBytes->bytes, mesh->GetIndices().data(), pIndiceBytes->size);

	// submesh信息
	std::vector<SubMeshMessage>* pSubMeshInfos = new std::vector<SubMeshMessage>();
	meshMessage.subMeshInfos.arg = pSubMeshInfos;
	for (int i = 0; i < mesh->GetSubMeshCount(); i ++)
	{
		SubMeshMessage subMesh = SubMeshMessage_init_default;
		const SubMeshInfo& subMeshInfo = mesh->GetSubMeshInfo(i);
		subMesh.firstIndex = subMeshInfo.firstIndex;
		subMesh.indexCount = subMeshInfo.indexCount;
		subMesh.topology = (PrimitiveType)subMeshInfo.topology;
		subMesh.vertexCount = subMeshInfo.vertexCount;
		pSubMeshInfos->push_back(subMesh);
	}
	meshMessage.subMeshInfos.funcs.encode = nanopb_encode_gnx_submeshinfo;

	// 顶点通道信息
	std::vector<VertexChannelMessage>* pVertexChannels = new std::vector<VertexChannelMessage>();
	meshMessage.vertexChannelInfos.arg = pVertexChannels;
	for (int i = 0; i < kShaderChannelCount; i ++)
	{
		const ChannelInfo& channelInfo = vertexData.GetChannel(i);
		if (!channelInfo.IsValid())
		{
			continue;
		}

		VertexChannelMessage vertexChannel = VertexChannelMessage_init_default;
		vertexChannel.offset = channelInfo.offset;
		vertexChannel.stride = channelInfo.stride;
		vertexChannel.format = (VertexDataFormat)channelInfo.format;
		vertexChannel.vertexChannel = (VertexChannel)i;

		pVertexChannels->push_back(vertexChannel);
	}
	meshMessage.vertexChannelInfos.funcs.encode = nanopb_encode_gnx_vertex_channel_info;

	size_t encodedSize = 0;
	pb_get_encoded_size(&encodedSize, MeshMessage_fields, &meshMessage);
	ByteVectorPtr buffer = std::make_shared<ByteVector>();
	buffer->resize(encodedSize);

	// 编码
	pb_ostream_t enc_stream;
	enc_stream = pb_ostream_from_buffer(buffer->data(), buffer->size());
	if (!pb_encode(&enc_stream, MeshMessage_fields, &meshMessage))
	{
		//encode error happened
		LOG_INFO("pb encode error in %s [%s]\n", __func__, PB_GET_ERROR(&enc_stream));
		buffer->clear();
		return buffer;
	}

	buffer->resize(enc_stream.bytes_written);

	return buffer;
}

NS_ASSETPROCESS_END
