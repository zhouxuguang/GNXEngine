#ifndef GNX_ENGINE_TEXTURE_ASSET_INCLUDE
#define GNX_ENGINE_TEXTURE_ASSET_INCLUDE

#include "Asset.h"
#include "TextureMessage.pb.h"
#include "Runtime/RenderCore/include/TextureFormat.h"
#include <vector>

NS_ASSETMANAGER_BEGIN

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

	/**
	 * 获取纹理数据
	 */
	const uint8_t* GetData() const;

	/**
	 * 获取纹理数据大小
	 */
	uint32_t GetDataSize() const;

    /**
     * 获取纹理格式
     */
    RenderCore::TextureFormat GetFormat() const;

    /**
     * 获取纹理类型（2D/3D/CUBE/2D_ARRAY）
     */
    RenderCore::TextureType GetTextureType() const;

	// 从文件加载数据,pb格式
	bool LoadFromFile(const std::string& filePath);

	bool LoadFromMemory(const void* pData, size_t dataSize);
private:
	ByteVector mTextureData;

	bool mIsOnGPU = false;

	// GPU 句柄（RenderSystem会实现）
	void* mGpuHandle;

	RenderCore::TextureType mTextureType = RenderCore::TextureType_Unkown;
	RenderCore::TextureFormat mTextureFormat;
	uint32_t mWidth = 0;
	uint32_t mHeight = 0;
	uint32_t mMipLevels = 0;
	uint32_t mDepth = 0;
	uint32_t mArrayLayers = 0;

	void ParseMeta(const ByteVector& binData);
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_TEXTURE_ASSET_INCLUDE
