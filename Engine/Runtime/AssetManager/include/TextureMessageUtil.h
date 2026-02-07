#ifndef GNX_ENGINE_TEXTUREMESSAGE_UTIL_INCLUDE_SDGMJFKJ
#define GNX_ENGINE_TEXTUREMESSAGE_UTIL_INCLUDE_SDGMJFKJ

#include "AssetDefine.h"
#include <vector>
#include <cstdint>

NS_ASSETMANAGER_BEGIN

/**
 * TextureMessage 工具类
 * 提供 TextureMessage 的序列化和反序列化功能
 */
class TextureMessageUtil
{
public:
	/**
	 * 反序列化 TextureMessage
	 * @param pData protobuf 数据指针
	 * @param dataSize 数据大小
	 * @param textureData 输出的纹理数据
	 * @return 是否成功
	 */
	static bool DecodeTextureMessage(const uint8_t* pData, uint32_t dataSize, ByteVector& textureData);

	/**
	 * 序列化 TextureMessage
	 * @param imageData 图像数据（KTX格式）
	 * @param dataSize 数据大小
	 * @return 序列化后的 protobuf 数据
	 */
	static ByteVectorPtr EncodeTextureMessage(const uint8_t* imageData, uint32_t dataSize);

private:
	TextureMessageUtil();
	~TextureMessageUtil();
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_TEXTUREMESSAGE_UTIL_INCLUDE_SDGMJFKJ
