#include "TextureMetaData.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>

NS_ASSETMANAGER_BEGIN

TextureMetaData::TextureMetaData()
{
	// 初始化为默认值
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

	// 设置默认的引擎版本
	m_engineVersion = "1.0.0";
}

TextureMetaData::~TextureMetaData()
{
}

void TextureMetaData::SetSize(uint32_t width, uint32_t height)
{
	SetSize(width, height, 1);
}

void TextureMetaData::SetSize(uint32_t width, uint32_t height, uint32_t depth)
{
	m_message.width = width;
	m_message.height = height;
	m_message.depth = depth;
}

void TextureMetaData::SetMipLevels(uint32_t levels)
{
	m_message.mipLevels = levels;
}

void TextureMetaData::SetArrayLayers(uint32_t layers)
{
	m_message.arrayLayers = layers;
}

void TextureMetaData::SetPixelFormat(imagecodec::ImagePixelFormat format)
{
	m_message.format = ConvertPixelFormat(format);
	m_message.isSRGB = DetectSRGB(format);
	m_message.hasAlpha = DetectAlpha(format);

	// 检测是否为压缩格式
	m_message.isCompressed = (format >= imagecodec::FORMAT_EAC_R);
}

void TextureMetaData::SetTextureType(TextureType type)
{
	m_message.textureType = type;
}

void TextureMetaData::SetSourceFile(const std::string& sourceFile)
{
	m_sourceFile = sourceFile;
}

void TextureMetaData::SetDataSize(uint32_t size)
{
	m_message.dataSize = size;
}

void TextureMetaData::SetIsSRGB(bool isSRGB)
{
	m_message.isSRGB = isSRGB;
}

void TextureMetaData::SetHasAlpha(bool hasAlpha)
{
	m_message.hasAlpha = hasAlpha;
}

void TextureMetaData::SetIsCubemap(bool isCubemap)
{
	m_message.isCubemap = isCubemap;
}

void TextureMetaData::SetIsCompressed(bool isCompressed)
{
	m_message.isCompressed = isCompressed;
}

uint32_t TextureMetaData::GetWidth() const
{
	return m_message.width;
}

uint32_t TextureMetaData::GetHeight() const
{
	return m_message.height;
}

uint32_t TextureMetaData::GetDepth() const
{
	return m_message.depth;
}

uint32_t TextureMetaData::GetMipLevels() const
{
	return m_message.mipLevels;
}

uint32_t TextureMetaData::GetArrayLayers() const
{
	return m_message.arrayLayers;
}

imagecodec::ImagePixelFormat TextureMetaData::GetPixelFormat() const
{
	// 反向转换protobuf的PixelFormat到VImage的PixelFormat
	switch (m_message.format)
	{
	case PixelFormat_GRAY8:
		return imagecodec::FORMAT_GRAY8;
	case PixelFormat_GRAY8_ALPHA8:
		return imagecodec::FORMAT_GRAY8_ALPHA8;
	case PixelFormat_RGBA8:
		return imagecodec::FORMAT_RGBA8;
	case PixelFormat_RGB8:
		return imagecodec::FORMAT_RGB8;
	case PixelFormat_RGBA32F:
		return imagecodec::FORMAT_RGBA32Float;
	case PixelFormat_RGB32F:
		return imagecodec::FORMAT_RGB32Float;
	case PixelFormat_SRGB8:
		return imagecodec::FORMAT_SRGB8;
	case PixelFormat_SRGB8_ALPHA8:
		return imagecodec::FORMAT_SRGB8_ALPHA8;
	default:
		return imagecodec::FORMAT_UNKNOWN;
	}
}

TextureType TextureMetaData::GetTextureType() const
{
	return m_message.textureType;
}

std::string TextureMetaData::GetSourceFile() const
{
	return m_sourceFile;
}

uint32_t TextureMetaData::GetDataSize() const
{
	return m_message.dataSize;
}

bool TextureMetaData::IsSRGB() const
{
	return m_message.isSRGB;
}

bool TextureMetaData::HasAlpha() const
{
	return m_message.hasAlpha;
}

bool TextureMetaData::IsCubemap() const
{
	return m_message.isCubemap;
}

bool TextureMetaData::IsCompressed() const
{
	return m_message.isCompressed;
}

void TextureMetaData::FillFromImage(const imagecodec::VImagePtr& image)
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

void TextureMetaData::AutoDetectTextureType(const std::string& fileName)
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

bool TextureMetaData::SaveToFile(const std::string& filePath)
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

bool TextureMetaData::LoadFromFile(const std::string& filePath)
{
	// 清空现有数据
	m_sourceFile.clear();
	m_importTime.clear();
	m_engineVersion.clear();

	// 设置字符串回调
	SetupStringCallbacks();

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

const TextureMessage& TextureMetaData::GetMessage() const
{
	return m_message;
}

PixelFormat TextureMetaData::ConvertPixelFormat(imagecodec::ImagePixelFormat format)
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

bool TextureMetaData::DetectSRGB(imagecodec::ImagePixelFormat format)
{
	return (format == imagecodec::FORMAT_SRGB8 ||
	        format == imagecodec::FORMAT_SRGB8_ALPHA8);
}

bool TextureMetaData::DetectAlpha(imagecodec::ImagePixelFormat format)
{
	return (format == imagecodec::FORMAT_RGBA8 ||
	        format == imagecodec::FORMAT_SRGB8_ALPHA8 ||
	        format == imagecodec::FORMAT_GRAY8_ALPHA8 ||
	        format == imagecodec::FORMAT_RGBA32Float ||
	        format == imagecodec::FORMAT_RGBA4444 ||
	        format == imagecodec::FORMAT_RGB5A1);
}

	uint64_t TextureMetaData::CalculateHash(const uint8_t* data, size_t size)
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
static bool string_encode_callback(pb_ostream_t *stream, const pb_field_t *field, const void *arg)
{
	const std::string* str = static_cast<const std::string*>(arg);
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

void TextureMetaData::SetupStringCallbacks()
{
	// 设置 sourceFile 的回调
	//m_message.sourceFile.funcs.encode = string_encode_callback;
	m_message.sourceFile.funcs.decode = string_decode_callback;
	m_message.sourceFile.arg = const_cast<std::string*>(&m_sourceFile);

	// 设置 importTime 的回调
	//m_message.importTime.funcs.encode = string_encode_callback;
	m_message.importTime.funcs.decode = string_decode_callback;
	m_message.importTime.arg = const_cast<std::string*>(&m_importTime);

	// 设置 engineVersion 的回调
	//m_message.engineVersion.funcs.encode = string_encode_callback;
	m_message.engineVersion.funcs.decode = string_decode_callback;
	m_message.engineVersion.arg = const_cast<std::string*>(&m_engineVersion);
}

NS_ASSETMANAGER_END
