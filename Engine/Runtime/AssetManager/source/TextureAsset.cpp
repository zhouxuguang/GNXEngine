#include "TextureAsset.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>

// nanopb 头文件
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

NS_ASSETMANAGER_BEGIN

TextureAsset::TextureAsset()
	: m_dataSize(0)
	, m_isOnGPU(false)
	, m_gpuHandle(nullptr)
{
	// 初始化protobuf消息为默认值
	m_message = TextureMessage_init_default;
	m_message.compressType = CompressType_NONE;
	m_message.textureType = TextureType_Default;
	m_message.width = 0;
	m_message.height = 0;
	m_message.depth = 1;
	m_message.mipLevels = 1;
	m_message.arrayLayers = 1;
	m_message.format = PixelFormat_UNKNOWN;
	m_message.bytesPerPixel = 0;
	m_message.isSRGB = false;
	m_message.hasAlpha = false;
	m_message.isCubemap = false;
	m_message.isCompressed = false;
	m_message.dataSize = 0;
	m_message.hash = 0;
	
	// 设置默认引擎版本
	m_engineVersion = "1.0.0";
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
	// 从磁盘加载纹理元数据（.meta文件）
	if (!LoadFromFile(m_filePath + ".meta"))
	{
		return false;
	}

	// 读取纹理数据（.ktx等）
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
	return m_message.width;
}

uint32_t TextureAsset::GetHeight() const
{
	return m_message.height;
}

uint32_t TextureAsset::GetDepth() const
{
	return m_message.depth;
}

uint32_t TextureAsset::GetMipLevels() const
{
	return m_message.mipLevels;
}

uint32_t TextureAsset::GetArrayLayers() const
{
	return m_message.arrayLayers;
}

PixelFormat TextureAsset::GetFormat() const
{
	return m_message.format;
}

uint32_t TextureAsset::GetBytesPerPixel() const
{
	if (m_message.width > 0 && m_message.height > 0)
	{
		return static_cast<uint32_t>(m_dataSize / (m_message.width * m_message.height));
	}
	return 0;
}

TextureType TextureAsset::GetTextureType() const
{
	return m_message.textureType;
}

CompressType TextureAsset::GetCompressType() const
{
	return static_cast<CompressType>(m_message.compressType);
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
	return m_message.isSRGB;
}

bool TextureAsset::HasAlpha() const
{
	return m_message.hasAlpha;
}

bool TextureAsset::IsCubemap() const
{
	return m_message.isCubemap;
}

bool TextureAsset::IsCompressed() const
{
	return m_message.isCompressed;
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

// ==================== 元数据操作 ====================

void TextureAsset::SetSize(uint32_t width, uint32_t height)
{
	SetSize(width, height, 1);
}

void TextureAsset::SetSize(uint32_t width, uint32_t height, uint32_t depth)
{
	m_message.width = width;
	m_message.height = height;
	m_message.depth = depth;
}

void TextureAsset::SetMipLevels(uint32_t levels)
{
	m_message.mipLevels = levels;
}

void TextureAsset::SetArrayLayers(uint32_t layers)
{
	m_message.arrayLayers = layers;
}

void TextureAsset::SetPixelFormat(imagecodec::ImagePixelFormat format)
{
	m_message.format = ConvertPixelFormat(format);
	m_message.isSRGB = DetectSRGB(format);
	m_message.hasAlpha = DetectAlpha(format);
	
	// 检测是否为压缩格式
	m_message.isCompressed = (format >= imagecodec::FORMAT_EAC_R);
}

void TextureAsset::SetTextureType(TextureType type)
{
	m_message.textureType = type;
}

void TextureAsset::SetSourceFile(const std::string& sourceFile)
{
	m_sourceFile = sourceFile;
}

void TextureAsset::SetDataSize(uint32_t size)
{
	m_message.dataSize = size;
}

void TextureAsset::SetHash(uint64_t hash)
{
	m_message.hash = hash;
}

void TextureAsset::SetIsSRGB(bool isSRGB)
{
	m_message.isSRGB = isSRGB;
}

void TextureAsset::SetHasAlpha(bool hasAlpha)
{
	m_message.hasAlpha = hasAlpha;
}

void TextureAsset::SetIsCubemap(bool isCubemap)
{
	m_message.isCubemap = isCubemap;
}

void TextureAsset::SetIsCompressed(bool isCompressed)
{
	m_message.isCompressed = isCompressed;
}

void TextureAsset::FillFromImage(const imagecodec::VImagePtr& image)
{
	if (!image)
	{
		return;
	}
	
	SetSize(image->GetWidth(), image->GetHeight());
	SetPixelFormat(image->GetFormat());
	SetMipLevels(image->GetMipCount());
	
	// 设置字节数
	m_message.bytesPerPixel = image->GetBytesPerPixels();
}

void TextureAsset::AutoDetectTextureType(const std::string& fileName)
{
	// 转换为小写
	std::string lowerFileName = fileName;
	std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);
	
	// 基于文件名模式匹配检测纹理类型
	if (lowerFileName.find("_normal") != std::string::npos ||
	    lowerFileName.find("_n.") != std::string::npos)
	{
		SetTextureType(TextureType_Normal);
	}
	else if (lowerFileName.find("_diffuse") != std::string::npos ||
	         lowerFileName.find("_albedo") != std::string::npos ||
	         lowerFileName.find("_color") != std::string::npos ||
	         lowerFileName.find("_basecolor") != std::string::npos)
	{
		SetTextureType(TextureType_Albedo);
	}
	else if (lowerFileName.find("_roughness") != std::string::npos ||
	         lowerFileName.find("_r.") != std::string::npos)
	{
		SetTextureType(TextureType_Roughness);
	}
	else if (lowerFileName.find("_metallic") != std::string::npos ||
	         lowerFileName.find("_metalness") != std::string::npos ||
	         lowerFileName.find("_m.") != std::string::npos)
	{
		SetTextureType(TextureType_Metallic);
	}
	else if (lowerFileName.find("_specular") != std::string::npos ||
	         lowerFileName.find("_s.") != std::string::npos)
	{
		SetTextureType(TextureType_Specular);
	}
	else if (lowerFileName.find("_ambient") != std::string::npos ||
	         lowerFileName.find("_ao") != std::string::npos ||
	         lowerFileName.find("_occlusion") != std::string::npos)
	{
		SetTextureType(TextureType_Occlusion);
	}
	else if (lowerFileName.find("_emissive") != std::string::npos ||
	         lowerFileName.find("_emission") != std::string::npos ||
	         lowerFileName.find("_e.") != std::string::npos)
	{
		SetTextureType(TextureType_Emissive);
	}
	else if (lowerFileName.find("_height") != std::string::npos ||
	         lowerFileName.find("_displacement") != std::string::npos ||
	         lowerFileName.find("_bump") != std::string::npos)
	{
		SetTextureType(TextureType_Height);
	}
	else if (lowerFileName.find("_cubemap") != std::string::npos ||
	         lowerFileName.find("_cube") != std::string::npos)
	{
		SetTextureType(TextureType_Cubemap);
	}
	else
	{
		// 默认类型
		SetTextureType(TextureType_Default);
	}
}

bool TextureAsset::SaveToFile(const std::string& filePath)
{
	// 设置导入时间戳
	auto now = std::time(nullptr);
	auto tm = *std::localtime(&now);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	const_cast<std::string&>(m_importTime) = oss.str();
	
	// 设置字符串回调
	SetupStringCallbacks();
	
	// 序列化protobuf消息
	uint8_t buffer[4096];
	pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
	
	if (!pb_encode(&stream, TextureMessage_fields, &m_message))
	{
		// 编码失败
		return false;
	}
	
	// 写入文件
	std::ofstream outFile(filePath, std::ios::binary);
	if (!outFile.is_open())
	{
		return false;
	}
	
	outFile.write(reinterpret_cast<const char*>(buffer), stream.bytes_written);
	outFile.close();
	
	return true;
}

bool TextureAsset::LoadFromFile(const std::string& filePath)
{
	// 清空现有数据
	m_sourceFile.clear();
	m_importTime.clear();
	m_engineVersion.clear();
	m_imageData.clear();
	
	// 设置字符串回调
	SetupStringCallbacks();
	
	// 设置imageData解码回调
	m_message.imageData.funcs.decode = [](pb_istream_t* stream, const pb_field_t* field, void**arg) -> bool {
		std::vector<uint8_t>* dataPtr = static_cast<std::vector<uint8_t>*>(*arg);
		size_t length = stream->bytes_left;
		dataPtr->resize(length);
		return pb_read(stream, dataPtr->data(), length);
	};
	m_message.imageData.arg = &m_imageData;
	
	// 读取文件
	std::vector<uint8_t> data = baselib::FileUtil::ReadBinaryFile(filePath);
	if (data.empty())
	{
		return false;
	}
	
	// 反序列化protobuf消息
	pb_istream_t stream = pb_istream_from_buffer(data.data(), data.size());
	if (!pb_decode(&stream, TextureMessage_fields, &m_message))
	{
		// 解码失败
		return false;
	}
	
	return true;
}

void TextureAsset::SetImageData(const uint8_t* data, uint32_t size)
{
	m_imageData.assign(data, data + size);
	
	// 设置编码回调
	m_message.imageData.funcs.encode = [](pb_ostream_t* stream, const pb_field_t* field, void * const * arg) -> bool {
		const std::vector<uint8_t>* dataPtr = static_cast<const std::vector<uint8_t>*>(*arg);
		if (!pb_encode_tag_for_field(stream, field))
		{
			return false;
		}
		return pb_encode_string(stream, dataPtr->data(), dataPtr->size());
	};
	m_message.imageData.funcs.decode = [](pb_istream_t* stream, const pb_field_t* field, void**arg) -> bool {
		std::vector<uint8_t>* dataPtr = static_cast<std::vector<uint8_t>*>(*arg);
		size_t length = stream->bytes_left;
		dataPtr->resize(length);
		return pb_read(stream, dataPtr->data(), length);
	};
	m_message.imageData.arg = &m_imageData;
	
	// 更新数据大小
	SetDataSize(size);
}

const uint8_t* TextureAsset::GetImageData() const
{
	return m_imageData.data();
}

uint32_t TextureAsset::GetImageDataSize() const
{
	return static_cast<uint32_t>(m_imageData.size());
}

const TextureMessage& TextureAsset::GetMessage() const
{
	return m_message;
}

// ==================== 工厂方法 ====================

TextureAsset* TextureAsset::CreateFromFiles(const std::string& metaPath, const std::string& dataPath)
{
	TextureAsset* asset = new TextureAsset();
	asset->m_filePath = dataPath;
	
	// 加载元数据
	if (!asset->LoadFromFile(metaPath))
	{
		delete asset;
		return nullptr;
	}
	
	// 设置GUID和名称（统一使用hash作为GUID）
	if (asset->m_message.hash != 0)
	{
		asset->m_guid = std::to_string(asset->m_message.hash);
	}
	else
	{
		// 如果没有hash，回退到使用sourceFile
		asset->m_guid = asset->m_sourceFile;
	}
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

TextureAsset* TextureAsset::CreateFromTextureMessageFile(const std::string& textureFilePath)
{
	TextureAsset* asset = new TextureAsset();
	asset->m_filePath = textureFilePath;
	
	// 加载元数据（包含KTX数据）
	if (!asset->LoadFromFile(textureFilePath))
	{
		delete asset;
		return nullptr;
	}
	
	// 从消息中获取KTX数据
	const TextureMessage& msg = asset->m_message;
	
	// 提取KTX数据
	const uint8_t* imageData = asset->GetImageData();
	uint32_t imageDataSize = asset->GetImageDataSize();
	
	if (imageData && imageDataSize > 0)
	{
		asset->m_textureData.assign(imageData, imageData + imageDataSize);
		asset->m_dataSize = imageDataSize;
	}
	else
	{
		// imageData为空，说明导入时没有正确保存KTX数据
		//std::cerr << "Texture has no image data: " << textureFilePath << std::endl;
		delete asset;
		return nullptr;
	}
	
	// 设置GUID和名称
	asset->m_guid = std::to_string(msg.hash);
	asset->m_name = baselib::FileName(textureFilePath).GetFileNoExtension();
	
	asset->SetMemorySize(asset->m_dataSize);
	asset->SetLastModified(0);
	asset->SetState(AssetState::Loaded);
	
	return asset;
}

TextureAsset* TextureAsset::CreateFromVImage(const imagecodec::VImagePtr& image, const std::string& name)
{
	TextureAsset* asset = new TextureAsset();
	
	// 从VImage填充元数据
	asset->FillFromImage(image);
	asset->SetSourceFile(name);
	
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

// ==================== 辅助函数 ====================

PixelFormat TextureAsset::ConvertPixelFormat(imagecodec::ImagePixelFormat format)
{
	switch (format)
	{
	case imagecodec::FORMAT_GRAY8:
		return PixelFormat_GRAY8;
	case imagecodec::FORMAT_GRAY8_ALPHA8:
		return PixelFormat_GRAY8_ALPHA8;
	case imagecodec::FORMAT_RGBA8:
		return PixelFormat_RGBA8;
	case imagecodec::FORMAT_RGB8:
		return PixelFormat_RGB8;
	case imagecodec::FORMAT_RGBA32Float:
		return PixelFormat_RGBA32F;
	case imagecodec::FORMAT_RGB32Float:
		return PixelFormat_RGB32F;
	case imagecodec::FORMAT_SRGB8:
		return PixelFormat_SRGB8;
	case imagecodec::FORMAT_SRGB8_ALPHA8:
		return PixelFormat_SRGB8_ALPHA8;
	default:
		return PixelFormat_UNKNOWN;
	}
}

bool TextureAsset::DetectSRGB(imagecodec::ImagePixelFormat format)
{
	return (format == imagecodec::FORMAT_SRGB8 ||
	        format == imagecodec::FORMAT_SRGB8_ALPHA8);
}

bool TextureAsset::DetectAlpha(imagecodec::ImagePixelFormat format)
{
	return (format == imagecodec::FORMAT_RGBA8 ||
	        format == imagecodec::FORMAT_SRGB8_ALPHA8 ||
	        format == imagecodec::FORMAT_GRAY8_ALPHA8 ||
	        format == imagecodec::FORMAT_RGBA32Float ||
	        format == imagecodec::FORMAT_RGBA4444 ||
	        format == imagecodec::FORMAT_RGB5A1);
}

uint64_t TextureAsset::CalculateHash(const uint8_t* data, size_t size)
{
	if (!data || size == 0)
	{
		return 0;
	}
	
	// 使用SHA256作为哈希
	baselib::SHA256 sha;
	sha.update(data, size);
	std::array<uint8_t, 32> digest = sha.digest();
	
	// 取前8个字节作为64位哈希值
	uint64_t hash = 0;
	for (int i = 0; i < 8 && i < digest.size(); i++)
	{
		hash = (hash << 8) | digest[i];
	}
	
	return hash;
}

// 字符串编码回调
static bool string_encode_callback(pb_ostream_t *stream, const pb_field_t *field, void * const * arg)
{
	const std::string* str = static_cast<const std::string*>(*arg);
	if (!str || str->empty())
	{
		return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)"", 0);
	}
	return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)str->c_str(), str->length());
}

// 字符串解码回调
static bool string_decode_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	std::string* str = static_cast<std::string*>(*arg);
	if (!str)
	{
		return false;
	}
	
	size_t length = stream->bytes_left;
	if (length > 1024)  // 限制最大长度
	{
		return false;
	}
	
	str->resize(length);
	if (!pb_read(stream, (uint8_t*)str->data(), length))
	{
		return false;
	}
	
	return true;
}

void TextureAsset::SetupStringCallbacks()
{
	// 设置 sourceFile 的回调
	m_message.sourceFile.funcs.encode = string_encode_callback;
	m_message.sourceFile.funcs.decode = string_decode_callback;
	m_message.sourceFile.arg = const_cast<std::string*>(&m_sourceFile);
	
	// 设置 importTime 的回调
	m_message.importTime.funcs.encode = string_encode_callback;
	m_message.importTime.funcs.decode = string_decode_callback;
	m_message.importTime.arg = const_cast<std::string*>(&m_importTime);
	
	// 设置 engineVersion 的回调
	m_message.engineVersion.funcs.encode = string_encode_callback;
	m_message.engineVersion.funcs.decode = string_decode_callback;
	m_message.engineVersion.arg = const_cast<std::string*>(&m_engineVersion);
}

NS_ASSETMANAGER_END
