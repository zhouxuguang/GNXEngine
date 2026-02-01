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

	ByteVectorPtr buffer = std::make_shared<ByteVector>();
	return buffer;
}

NS_ASSETMANAGER_END
