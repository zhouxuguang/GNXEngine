#include "MeshMessageUtil.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "MeshMessage.pb.h"

NS_ASSETPROCESS_BEGIN

bool nanopb_decode_gnx_bytes(pb_istream_t* stream, const pb_field_t* field, void** arg)
{
	if (*arg != NULL)
	{
		pb_bytes_array_t* pByteArray = (pb_bytes_array_t*)*arg;
		if (pByteArray)
		{
			free(pByteArray);
		}
		pByteArray->size = 0;
		*arg = NULL;
	}

	size_t alloc_size = stream->bytes_left;
	pb_bytes_array_t* pByteArray = (pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(alloc_size));
	if (!pByteArray)
		return false;

	pByteArray->size = alloc_size;

	bool status = pb_read(stream, pByteArray->bytes, pByteArray->size);
	*arg = pByteArray;

	return status;
}

bool nanopb_encode_gnx_bytes(pb_ostream_t* stream, const pb_field_t* field, void* const* arg)
{
	if (!*arg)
	{
		return true;
	}

	if (!pb_encode_tag_for_field(stream, field))
		return false;

	pb_bytes_array_t* pByteArray = (pb_bytes_array_t*)*arg;

	bool status = pb_write(stream, pByteArray->bytes, pByteArray->size);

	return status;
}

MeshMessageUtil::MeshMessageUtil()
{
}

MeshMessageUtil::~MeshMessageUtil()
{
}

bool MeshMessageUtil::DecodeMeshMessage(const uint8_t* pData, uint32_t dataSize, Mesh* mesh)
{
	if (!pData || dataSize <= 0)
	{
		return false;
	}
	return false;
}

ByteVectorPtr MeshMessageUtil::EncodeMeshMessage(const Mesh* mesh)
{
	//pb_get_encoded_size()
	// 1 先填充结构体的值 2 获得编码后数据大小，分配编码后空间 3 编码
	MeshMessage meshMessage = MeshMessage_init_zero;

	return ByteVectorPtr();
}

NS_ASSETPROCESS_END