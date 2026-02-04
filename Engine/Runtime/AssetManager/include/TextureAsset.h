#ifndef GNX_ENGINE_TEXTURE_ASSET_INCLUDE
#define GNX_ENGINE_TEXTURE_ASSET_INCLUDE

#include "Asset.h"
#include "TextureMessage.pb.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include <vector>

NS_ASSETMANAGER_BEGIN

// 纹理的文件头 https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html
struct TextureDataHeader
{
	char identifier[12];
	uint32_t endianness;
	uint32_t glType;
	uint32_t glTypeSize;
	uint32_t glFormat;
	uint32_t glInternalFormat;
	uint32_t glBaseInternalFormat; // e.g. GL_RGBA, GL_BGRA, GL_RED
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t numberOfArrayElements;
	uint32_t numberOfFaces;
	uint32_t numberOfMipmapLevels;
	uint32_t bytesOfKeyValueData;
};

/**
 * 纹理资源类
 * 继承自Asset基类，提供纹理特定的功能
 */
class ASSET_MANAGER_API TextureAsset : public Asset
{
public:
	TextureAsset();
	virtual ~TextureAsset();

	// ==================== Asset基类接口 ====================

	virtual AssetType GetType() const override;
	virtual const std::string& GetGUID() const override;
	virtual const std::string& GetName() const override;
	virtual const std::string& GetFilePath() const override;

	virtual bool Load() override;
	virtual void Unload() override;
	virtual bool Reload() override;
	virtual bool IsLoaded() const override;

	virtual bool UploadToGPU() override;
	virtual void ReleaseFromGPU() override;
	virtual bool IsOnGPU() const override;

	// ==================== 纹理属性 ====================

	/**
	 * 获取纹理宽度
	 */
	uint32_t GetWidth() const;

	/**
	 * 获取纹理高度
	 */
	uint32_t GetHeight() const;

	/**
	 * 获取纹理深度（1表示2D纹理）
	 */
	uint32_t GetDepth() const;

	/**
	 * 获取Mipmap级数
	 */
	uint32_t GetMipLevels() const;

	/**
	 * 获取数组层数（纹理数组）
	 */
	uint32_t GetArrayLayers() const;

	/**
	 * 获取每像素字节数
	 */
	uint32_t GetBytesPerPixel() const;

	// ==================== 纹理数据访问 ====================

	/**
	 * 获取纹理数据
	 */
	const uint8_t* GetData() const;

	/**
	 * 获取纹理数据大小
	 */
	uint32_t GetDataSize() const;

	/**
	 * 设置纹理数据
	 */
	void SetData(const uint8_t* data, uint32_t size);

	// ==================== 纹理属性 ====================

	/**
	 * 是否为sRGB色彩空间
	 */
	bool IsSRGB() const;

	/**
	 * 是否有Alpha通道
	 */
	bool HasAlpha() const;

	/**
	 * 是否为立方体贴图
	 */
	bool IsCubemap() const;

	/**
	 * 是否为压缩纹理
	 */
	bool IsCompressed() const;

	/**
	 * 检查纹理是否为法线贴图
	 */
	bool IsNormalMap() const;

	// ==================== 元数据操作 ====================

	// 设置纹理基本信息
	void SetSize(uint32_t width, uint32_t height);
	void SetSize(uint32_t width, uint32_t height, uint32_t depth);
	void SetMipLevels(uint32_t levels);
	void SetArrayLayers(uint32_t layers);
	void SetPixelFormat(imagecodec::ImagePixelFormat format);
	void SetSourceFile(const std::string& sourceFile);
	void SetDataSize(uint32_t size);
	void SetHash(uint64_t hash);
	void SetIsSRGB(bool isSRGB);
	void SetHasAlpha(bool hasAlpha);
	void SetIsCubemap(bool isCubemap);
	void SetIsCompressed(bool isCompressed);

	// 序列化为protobuf并保存到文件
	bool SaveToFile(const std::string& filePath);

	// 从文件加载数据,pb格式
	bool LoadFromFile(const std::string& filePath);

	// 设置图像数据（用于序列化）
	void SetImageData(const uint8_t* data, uint32_t size);

	// 获取图像数据（用于反序列化）
	const uint8_t* GetImageData() const;
	uint32_t GetImageDataSize() const;

private:
	ByteVector mTextureData;
	TextureDataHeader mTextureHeader;

	bool m_isOnGPU;

	// GPU 句柄（RenderSystem会实现）
	void* m_gpuHandle;
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_TEXTURE_ASSET_INCLUDE
