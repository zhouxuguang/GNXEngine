#include "TextureAsset.h"
#include "Runtime/BaseLib/include/BaseLib.h"

NS_ASSETMANAGER_BEGIN

TextureAsset::TextureAsset()
	: m_dataSize(0)
	, m_isOnGPU(false)
	, m_gpuHandle(nullptr)
{
}

TextureAsset::~TextureAsset()
{
	Unload();
	ReleaseFromGPU();
}

AssetType TextureAsset::GetType() const
{
	return AssetType::Texture;
}

const std::string& TextureAsset::GetGUID() const
{
	return m_guid;
}

const std::string& TextureAsset::GetName() const
{
	return m_name;
}

const std::string& TextureAsset::GetFilePath() const
{
	return m_filePath;
}

bool TextureAsset::Load()
{
	// 从磁盘加载纹理元数据
	if (!m_metaData.LoadFromFile(m_filePath + ".meta"))
	{
		return false;
	}

	// 读取纹理数据
	std::vector<uint8_t> data = baselib::FileUtil::ReadBinaryFile(m_filePath);
	if (data.empty())
	{
		SetState(AssetState::Error);
		return false;
	}

	// 保存纹理数据
	m_textureData = std::move(data);
	m_dataSize = static_cast<uint32_t>(m_textureData.size());

	// 更新元数据
	SetMemorySize(m_dataSize);
	SetLastModified(0); // TODO: 实现文件修改时间获取

	SetState(AssetState::Loaded);
	return true;
}

void TextureAsset::Unload()
{
	if (m_textureData.empty())
	{
		return;
	}

	m_textureData.clear();
	m_dataSize = 0;

	SetState(AssetState::Unloaded);
}

bool TextureAsset::Reload()
{
	Unload();
	return Load();
}

bool TextureAsset::IsLoaded() const
{
	return GetState() == AssetState::Loaded ||
	       GetState() == AssetState::Uploading ||
	       GetState() == AssetState::Ready;
}

bool TextureAsset::UploadToGPU()
{
	if (!IsLoaded())
	{
		return false;
	}

	// TODO: 调用RenderSystem上传纹理到GPU
	// 这里需要RenderSystem的支持

	m_isOnGPU = true;
	SetState(AssetState::Ready);
	return true;
}

void TextureAsset::ReleaseFromGPU()
{
	if (!m_isOnGPU)
	{
		return;
	}

	// TODO: 调用RenderSystem从GPU释放纹理
	// 这里需要RenderSystem的支持

	m_isOnGPU = false;
}

bool TextureAsset::IsOnGPU() const
{
	return m_isOnGPU;
}

uint32_t TextureAsset::GetWidth() const
{
	return m_metaData.GetWidth();
}

uint32_t TextureAsset::GetHeight() const
{
	return m_metaData.GetHeight();
}

uint32_t TextureAsset::GetDepth() const
{
	return m_metaData.GetDepth();
}

uint32_t TextureAsset::GetMipLevels() const
{
	return m_metaData.GetMipLevels();
}

uint32_t TextureAsset::GetArrayLayers() const
{
	return m_metaData.GetArrayLayers();
}

PixelFormat TextureAsset::GetFormat() const
{
	return m_metaData.GetMessage().format;
}

uint32_t TextureAsset::GetBytesPerPixel() const
{
	if (m_metaData.GetWidth() > 0 && m_metaData.GetHeight() > 0)
	{
		return static_cast<uint32_t>(m_dataSize / (m_metaData.GetWidth() * m_metaData.GetHeight()));
	}
	return 0;
}

TextureType TextureAsset::GetTextureType() const
{
	return m_metaData.GetTextureType();
}

CompressType TextureAsset::GetCompressType() const
{
	return static_cast<CompressType>(m_metaData.GetMessage().compressType);
}

const uint8_t* TextureAsset::GetData() const
{
	return m_textureData.data();
}

uint32_t TextureAsset::GetDataSize() const
{
	return m_dataSize;
}

void TextureAsset::SetData(const uint8_t* data, uint32_t size)
{
	m_textureData.assign(data, data + size);
	m_dataSize = size;
}

bool TextureAsset::IsSRGB() const
{
	return m_metaData.IsSRGB();
}

bool TextureAsset::HasAlpha() const
{
	return m_metaData.HasAlpha();
}

bool TextureAsset::IsCubemap() const
{
	return m_metaData.IsCubemap();
}

bool TextureAsset::IsCompressed() const
{
	return m_metaData.IsCompressed();
}

bool TextureAsset::SupportsLumen() const
{
	// Lumen需要使用多种纹理
	TextureType type = GetTextureType();
	return type == TextureType_Normal ||
	       type == TextureType_Albedo ||
	       type == TextureType_Roughness ||
	       type == TextureType_Metallic ||
	       type == TextureType_Occlusion ||
	       type == TextureType_Emissive;
}

bool TextureAsset::IsNormalMap() const
{
	return GetTextureType() == TextureType_Normal;
}

bool TextureAsset::IsAlbedoMap() const
{
	TextureType type = GetTextureType();
	return type == TextureType_Albedo || type == TextureType_Default;
}

bool TextureAsset::IsRoughnessMap() const
{
	return GetTextureType() == TextureType_Roughness;
}

bool TextureAsset::IsMetallicMap() const
{
	return GetTextureType() == TextureType_Metallic;
}

bool TextureAsset::IsOcclusionMap() const
{
	return GetTextureType() == TextureType_Occlusion;
}

bool TextureAsset::IsEmissiveMap() const
{
	return GetTextureType() == TextureType_Emissive;
}

TextureAsset* TextureAsset::CreateFromFiles(const std::string& metaPath, const std::string& dataPath)
{
	TextureAsset* asset = new TextureAsset();
	asset->m_filePath = dataPath;

	// 加载元数据
	if (!asset->m_metaData.LoadFromFile(metaPath))
	{
		delete asset;
		return nullptr;
	}

	// 设置GUID和名称
	asset->m_guid = asset->m_metaData.GetSourceFile();
	asset->m_name = baselib::FileName(dataPath).GetFileNoExtension();

	// 读取数据
	std::vector<uint8_t> data = baselib::FileUtil::ReadBinaryFile(dataPath);
	if (data.empty())
	{
		delete asset;
		return nullptr;
	}

	asset->m_textureData = std::move(data);
	asset->m_dataSize = static_cast<uint32_t>(asset->m_textureData.size());

	asset->SetMemorySize(asset->m_dataSize);
	asset->SetLastModified(0); // TODO: 实现文件修改时间获取
	asset->SetState(AssetState::Loaded);

	return asset;
}

TextureAsset* TextureAsset::CreateFromVImage(const imagecodec::VImagePtr& image, const std::string& name)
{
	TextureAsset* asset = new TextureAsset();

	// 从VImage填充元数据
	asset->m_metaData.FillFromImage(image);
	asset->m_metaData.SetSourceFile(name);

	// 生成GUID（使用文件名）
	asset->m_guid = name;
	asset->m_name = name;

	// 复制图像数据
	uint32_t dataSize = image->GetImageSize(0);
	const uint8_t* imageData = image->GetPixels(0);

	asset->m_textureData.assign(imageData, imageData + dataSize);
	asset->m_dataSize = dataSize;

	asset->SetMemorySize(asset->m_dataSize);
	asset->SetState(AssetState::Loaded);

	return asset;
}

const TextureMetaData& TextureAsset::GetMetaData() const
{
	return m_metaData;
}

void TextureAsset::SetMetaData(const TextureMetaData& metaData)
{
	m_metaData = metaData;
}

NS_ASSETMANAGER_END
