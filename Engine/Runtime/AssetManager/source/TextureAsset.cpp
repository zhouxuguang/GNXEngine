#include "TextureAsset.h"
#include "AssetFileHeader.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include "TextureMessageUtil.h"


NS_ASSETMANAGER_BEGIN

TextureAsset::TextureAsset() : 
	m_isOnGPU(false)
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

	SetState(AssetState::Loaded);
	return true;
}

void TextureAsset::Unload()
{

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
	return 0;
}

uint32_t TextureAsset::GetHeight() const
{
	return 0;
}

uint32_t TextureAsset::GetDepth() const
{
	return 0;
}

uint32_t TextureAsset::GetMipLevels() const
{
	return 0;
}

uint32_t TextureAsset::GetArrayLayers() const
{
	return 0;
}

uint32_t TextureAsset::GetBytesPerPixel() const
{
	return 0;
}

const uint8_t* TextureAsset::GetData() const
{
	return nullptr;
}

uint32_t TextureAsset::GetDataSize() const
{
	return 0;
}

void TextureAsset::SetData(const uint8_t* data, uint32_t size)
{
}

bool TextureAsset::IsSRGB() const
{
	return true;
}

bool TextureAsset::HasAlpha() const
{
	return true;
}

bool TextureAsset::IsCubemap() const
{
	return false;
}

bool TextureAsset::IsCompressed() const
{
	return false;
}

bool TextureAsset::IsNormalMap() const
{
	return false;
}

// ==================== 元数据操作 ====================

void TextureAsset::SetSize(uint32_t width, uint32_t height)
{
	SetSize(width, height, 1);
}

void TextureAsset::SetSize(uint32_t width, uint32_t height, uint32_t depth)
{
}

void TextureAsset::SetMipLevels(uint32_t levels)
{
}

void TextureAsset::SetArrayLayers(uint32_t layers)
{
}

void TextureAsset::SetPixelFormat(imagecodec::ImagePixelFormat format)
{
}

void TextureAsset::SetDataSize(uint32_t size)
{
	//m_message.dataSize = size;
}

void TextureAsset::SetHash(uint64_t hash)
{
	//m_message.hash = hash;
}

void TextureAsset::SetIsSRGB(bool isSRGB)
{
	//m_message.isSRGB = isSRGB;
}

void TextureAsset::SetHasAlpha(bool hasAlpha)
{
	//m_message.hasAlpha = hasAlpha;
}

void TextureAsset::SetIsCubemap(bool isCubemap)
{
	//m_message.isCubemap = isCubemap;
}

void TextureAsset::SetIsCompressed(bool isCompressed)
{
	//m_message.isCompressed = isCompressed;
}

bool TextureAsset::SaveToFile(const std::string& filePath)
{
	
	return true;
}

bool TextureAsset::LoadFromFile(const std::string& filePath)
{
	ByteVector imageData = baselib::FileUtil::ReadBinaryFile(filePath);
	if (imageData.empty())
	{
		return false;
	}

	size_t assetHeader = sizeof(AssetFileHeader);

	ByteVector packedImageData;
	bool suc = TextureMessageUtil::DecodeTextureMessage(imageData.data() + assetHeader, imageData.size() - assetHeader, packedImageData);
	if (!suc)
	{
		return false;
	}

	memcpy(&mTextureHeader, packedImageData.data(), sizeof(TextureDataHeader));

	size_t textureDataSize = packedImageData.size() - sizeof(TextureDataHeader);

	mTextureData.resize(textureDataSize);
	memcpy(mTextureData.data(), packedImageData.data() + sizeof(TextureDataHeader), textureDataSize);

	return true;
}

void TextureAsset::SetImageData(const uint8_t* data, uint32_t size)
{
}

const uint8_t* TextureAsset::GetImageData() const
{
    return nullptr;
}

NS_ASSETMANAGER_END
