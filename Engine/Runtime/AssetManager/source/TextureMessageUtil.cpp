#include "TextureMessageUtil.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "TextureMessage.pb.h"
#include "Runtime/BaseLib/include/LogService.h"
#include "PBUtils.h"

NS_ASSETMANAGER_BEGIN

TextureMessageUtil::TextureMessageUtil()
{
}

TextureMessageUtil::~TextureMessageUtil()
{
}

bool TextureMessageUtil::DecodeTextureMessage(const uint8_t* pData, uint32_t dataSize, TextureData* textureData)
{
	if (!pData || dataSize <= 0 || !textureData)
	{
		return false;
	}

	// TODO: 实现反序列化逻辑
	// 1. 初始化 TextureMessage
	// 2. 设置解码回调函数
	// 3. 执行 pb_decode
	// 4. 提取 imageData 和 dataSize

	TextureMessage textureMessage = TextureMessage_init_default;
	textureMessage.imageData.funcs.decode = nanopb_decode_gnx_bytes;

	// decode data
	pb_istream_t dec_stream = pb_istream_from_buffer(pData, dataSize);
	if (!pb_decode(&dec_stream, TextureMessage_fields, &textureMessage))
	{
		LOG_INFO("pb decode error in DecodeTextureMessage %s, error %s\n", __func__, dec_stream.errmsg);
		return false;
	}

	pb_bytes_array_t* pImageData = (pb_bytes_array_t*)textureMessage.imageData.arg;
	if (pImageData)
	{
		textureData->imageData.resize(pImageData->size);
		memcpy(textureData->imageData.data(), pImageData->bytes, pImageData->size);

		free(pImageData);
		textureMessage.imageData.arg = nullptr;  // 防止重复释放
	}
	pb_release(TextureMessage_fields, &textureMessage);

	return true;
}

ByteVectorPtr TextureMessageUtil::EncodeTextureMessage(const uint8_t* imageData, uint32_t dataSize)
{
	if (!imageData || dataSize <= 0)
	{
		return nullptr;
	}

	// TODO: 实现序列化逻辑
	// 1. 初始化 TextureMessage
	// 2. 设置 dataSize
	// 3. 设置 imageData 回调函数
	// 4. 计算 pb_get_encoded_size
	// 5. 分配 buffer
	// 6. 执行 pb_encode

	TextureMessage textureMessage = TextureMessage_init_default;

	// 顶点数据
	pb_bytes_array_t* pDataBytes = (pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(dataSize));
	pDataBytes->size = dataSize;
	textureMessage.imageData.arg = pDataBytes;
	textureMessage.imageData.funcs.encode = nanopb_encode_gnx_bytes;
	memcpy(pDataBytes->bytes, imageData, pDataBytes->size);


	size_t encodedSize = 0;
	pb_get_encoded_size(&encodedSize, TextureMessage_fields, &textureMessage);
	ByteVectorPtr buffer = std::make_shared<ByteVector>();
	buffer->resize(encodedSize);

	// 编码
	pb_ostream_t enc_stream = pb_ostream_from_buffer(buffer->data(), buffer->size());
	if (!pb_encode(&enc_stream, TextureMessage_fields, &textureMessage))
	{
		//encode error happened
		LOG_INFO("pb encode error in EncodeTextureMessage %s [%s]\n", __func__, PB_GET_ERROR(&enc_stream));
		buffer->clear();
		return buffer;
	}

	free(pDataBytes);
	textureMessage.imageData.arg = nullptr;
	pb_release(TextureMessage_fields, &textureMessage);

	buffer->resize(enc_stream.bytes_written);

	return buffer;
}

NS_ASSETMANAGER_END
