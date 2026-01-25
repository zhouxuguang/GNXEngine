#include "PBUtils.h"


NS_ASSETMANAGER_BEGIN

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

	// 这里要写入二进制的大小，要不然pb不知道这个二进制的大小
	bool status = pb_encode_varint(stream, pByteArray->size);
	if (!status)
	{
		return false;
	}
	status = pb_write(stream, pByteArray->bytes, pByteArray->size);

	return status;
}

NS_ASSETMANAGER_END
