#ifndef GNX_ENGINE_TEXTURE_METADATA_INCLUDE
#define GNX_ENGINE_TEXTURE_METADATA_INCLUDE

#include "AssetDefine.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include "TextureMessage.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include <string>
#include <vector>

NS_ASSETMANAGER_BEGIN

/**
 * 纹理元数据管理类
 * 负责纹理元数据的创建、序列化和保存
 */
class TextureMetaData
{
public:
	TextureMetaData();
	~TextureMetaData();

	// 设置纹理基本信息
	void SetSize(uint32_t width, uint32_t height);
	void SetSize(uint32_t width, uint32_t height, uint32_t depth);
	void SetMipLevels(uint32_t levels);
	void SetArrayLayers(uint32_t layers);
	void SetPixelFormat(imagecodec::ImagePixelFormat format);
	void SetTextureType(TextureType type);
	void SetSourceFile(const std::string& sourceFile);
	void SetDataSize(uint32_t size);
	void SetIsSRGB(bool isSRGB);
	void SetHasAlpha(bool hasAlpha);
	void SetIsCubemap(bool isCubemap);
	void SetIsCompressed(bool isCompressed);

	// 获取纹理基本信息
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;
	uint32_t GetMipLevels() const;
	uint32_t GetArrayLayers() const;
	imagecodec::ImagePixelFormat GetPixelFormat() const;
	TextureType GetTextureType() const;
	std::string GetSourceFile() const;
	uint32_t GetDataSize() const;
	bool IsSRGB() const;
	bool HasAlpha() const;
	bool IsCubemap() const;
	bool IsCompressed() const;

	// 从VImage填充元数据
	void FillFromImage(const imagecodec::VImagePtr& image);

	// 自动检测纹理类型（基于文件名）
	void AutoDetectTextureType(const std::string& fileName);

	// 序列化为protobuf并保存到文件
	bool SaveToFile(const std::string& filePath);

	// 从文件加载元数据
	bool LoadFromFile(const std::string& filePath);

	// 获取protobuf消息结构
	const TextureMessage& GetMessage() const;

private:
	TextureMessage m_message;

	// 字符串字段的存储（因为 nanopb 使用回调处理字符串）
	std::string m_sourceFile;
	std::string m_importTime;
	std::string m_engineVersion;

	// 辅助函数：将VImage的PixelFormat转换为protobuf的PixelFormat
	static PixelFormat ConvertPixelFormat(imagecodec::ImagePixelFormat format);

	// 辅助函数：检测颜色空间
	static bool DetectSRGB(imagecodec::ImagePixelFormat format);

	// 辅助函数：检测是否有Alpha通道
	static bool DetectAlpha(imagecodec::ImagePixelFormat format);

	// 辅助函数：计算哈希值
	static uint64_t CalculateHash(const uint8_t* data, size_t size);

	// 辅助函数：设置字符串回调（用于序列化）
	void SetupStringCallbacks();
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_TEXTURE_METADATA_INCLUDE
