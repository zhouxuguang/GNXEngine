#ifndef GNX_ENGINE_TEXTURE_ASSET_INCLUDE
#define GNX_ENGINE_TEXTURE_ASSET_INCLUDE

#include "Asset.h"
#include "TextureMetaData.h"
#include "Runtime/ImageCodec/include/VImage.h"
#include <vector>

NS_ASSETMANAGER_BEGIN

/**
 * 纹理资源类
 * 继承自Asset基类，提供纹理特定的功能
 */
class TextureAsset : public Asset
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
	 * 获取像素格式
	 */
	PixelFormat GetFormat() const;

	/**
	 * 获取每像素字节数
	 */
	uint32_t GetBytesPerPixel() const;

	/**
	 * 获取纹理类型
	 */
	TextureType GetTextureType() const;

	/**
	 * 获取压缩类型
	 */
	CompressType GetCompressType() const;

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

	// ==================== Lumen 相关 ====================

	/**
	 * 检查纹理是否支持Lumen
	 * Lumen需要使用法线、反照率、粗糙度等纹理
	 */
	virtual bool SupportsLumen() const override;

	/**
	 * 检查纹理是否为法线贴图
	 */
	bool IsNormalMap() const;

	/**
	 * 检查纹理是否为反照率贴图（Albedo）
	 */
	bool IsAlbedoMap() const;

	/**
	 * 检查纹理是否为粗糙度贴图（Roughness）
	 */
	bool IsRoughnessMap() const;

	/**
	 * 检查纹理是否为金属度贴图（Metallic）
	 */
	bool IsMetallicMap() const;

	/**
	 * 检查纹理是否为AO贴图
	 */
	bool IsOcclusionMap() const;

	/**
	 * 检查纹理是否为自发光贴图（Emissive）
	 */
	bool IsEmissiveMap() const;

	// ==================== 工厂方法 ====================

	/**
	 * 从元数据文件创建纹理资源
	 * @param metaPath 元数据文件路径（.meta文件）
	 * @param dataPath 纹理数据文件路径（.ktx等）
	 * @return 创建的纹理资源，失败返回nullptr
	 */
	static TextureAsset* CreateFromFiles(const std::string& metaPath, const std::string& dataPath);

	/**
	 * 从VImage创建纹理资源
	 * @param image VImage对象
	 * @param name 纹理名称
	 * @return 创建的纹理资源
	 */
	static TextureAsset* CreateFromVImage(const imagecodec::VImagePtr& image, const std::string& name);

	// ==================== 元数据 ====================

	/**
	 * 获取纹理元数据
	 */
	const TextureMetaData& GetMetaData() const;

	/**
	 * 设置纹理元数据
	 */
	void SetMetaData(const TextureMetaData& metaData);

private:
	TextureMetaData m_metaData;
	std::vector<uint8_t> m_textureData;
	uint32_t m_dataSize;

	bool m_isOnGPU;

	// GPU 句柄（RenderSystem会实现）
	void* m_gpuHandle;
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_TEXTURE_ASSET_INCLUDE
