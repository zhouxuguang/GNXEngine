#include "AssetReference.h"
#include "Runtime/AssetManager/include/AssetFileHeader.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <fstream>
#include <ctime>
#include <iomanip>

NS_ASSETPROCESS_BEGIN

AssetReference::AssetReference()
{
	// 初始化为默认值
	m_message = AssetReferenceMessage_init_default;
	m_message.hash = 0;
	m_message.assetType = AssetType_Unknown;

	// 设置默认的引擎版本
	m_engineVersion = "1.0.0";
}

AssetReference::~AssetReference()
{
}

void AssetReference::SetHash(uint64_t hash)
{
	m_message.hash = hash;
}

void AssetReference::SetOriginalFileName(const std::string& fileName)
{
	m_originalFileName = fileName;
}

void AssetReference::SetOriginalPath(const std::string& path)
{
	m_originalPath = path;
}

void AssetReference::SetAssetType(AssetType assetType)
{
	m_message.assetType = assetType;
}

uint64_t AssetReference::GetHash() const
{
	return m_message.hash;
}

std::string AssetReference::GetOriginalFileName() const
{
	return m_originalFileName;
}

std::string AssetReference::GetOriginalPath() const
{
	return m_originalPath;
}

AssetType AssetReference::GetAssetType() const
{
	return m_message.assetType;
}

static AssetManager::AssetType ConvertToAssetManagerType(AssetType protoType)
{
    switch (protoType)
    {
        case AssetType_Unknown:
            return AssetManager::AssetType::Unknown;
        case AssetType_Mesh:
            return AssetManager::AssetType::Mesh;
        case AssetType_Texture:
            return AssetManager::AssetType::Texture;
        default:
            return AssetManager::AssetType::Unknown;
    }
}

bool AssetReference::SaveToFile(const std::string& filePath)
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
	uint8_t buffer[1024];
	pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
	
	if (!pb_encode(&stream, AssetReferenceMessage_fields, &m_message))
	{
		// 编码失败
		return false;
	}
	
	// 创建资产文件头
	uint32_t flags = AssetManager::AssetFileFlags::NONE;
	// 计算protobuf数据的哈希
	uint64_t hash = AssetManager::AssetFileHeaderUtil::ComputeHash(buffer, stream.bytes_written);
	AssetManager::AssetFileHeader header = AssetManager::AssetFileHeaderUtil::CreateHeader(
		ConvertToAssetManagerType(m_message.assetType),
		m_originalFileName,
		hash,
		stream.bytes_written,
		flags
	);
	
	// 写入文件：先写文件头，再写protobuf数据
	std::ofstream outFile(filePath, std::ios::binary);
	if (!outFile.is_open())
	{
		return false;
	}
	
	// 写入文件头
	if (!AssetManager::AssetFileHeaderUtil::WriteHeader(outFile, header))
	{
		outFile.close();
		return false;
	}
	
	// 写入protobuf数据
	outFile.write(reinterpret_cast<const char*>(buffer), stream.bytes_written);
	outFile.close();
	
	return true;
}

bool AssetReference::LoadFromFile(const std::string& filePath)
{
	// 清空现有数据
	m_originalFileName.clear();
	m_originalPath.clear();
	m_importTime.clear();
	m_engineVersion.clear();
	
	// 设置字符串回调
	SetupStringCallbacks();
	
	// 读取整个文件
	std::vector<uint8_t> fileData = baselib::FileUtil::ReadBinaryFile(filePath);
	if (fileData.empty())
	{
		return false;
	}
	
	// 检查是否包含资产文件头
	const uint8_t* dataPtr = fileData.data();
	uint64_t dataSize = fileData.size();
	
	// 尝试读取文件头
	AssetManager::AssetFileHeader header;
	if (AssetManager::AssetFileHeaderUtil::ReadHeader(filePath, header))
	{
		// 文件头有效，数据从dataOffset开始
		if (header.dataOffset >= sizeof(AssetManager::AssetFileHeader) && header.dataOffset + header.dataSize <= dataSize)
		{
			dataPtr = fileData.data() + header.dataOffset;
			dataSize = header.dataSize;
		}
		// 如果偏移量无效，回退到原始数据（可能是旧格式文件）
	}
	
	// 反序列化protobuf消息
	pb_istream_t stream = pb_istream_from_buffer(dataPtr, dataSize);
	if (!pb_decode(&stream, AssetReferenceMessage_fields, &m_message))
	{
		// 解码失败
		return false;
	}
	
	return true;
}

const AssetReferenceMessage& AssetReference::GetMessage() const
{
	return m_message;
}

// 字符串编码回调
static bool asset_ref_string_encode_callback(pb_ostream_t* stream, const pb_field_t* field, void * const * arg)
{
	const std::string* str = static_cast<const std::string*>(*arg);
	if (!str || str->empty())
	{
		return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)"", 0);
	}
	return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, (const uint8_t*)str->c_str(), str->length());
}

// 字符串解码回调
static bool asset_ref_string_decode_callback(pb_istream_t* stream, const pb_field_t* field, void** arg)
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

void AssetReference::SetupStringCallbacks()
{
	// 设置 originalFileName 的回调
	m_message.originalFileName.funcs.encode = asset_ref_string_encode_callback;
	m_message.originalFileName.funcs.decode = asset_ref_string_decode_callback;
	m_message.originalFileName.arg = const_cast<std::string*>(&m_originalFileName);

	// 设置 originalPath 的回调
	m_message.originalPath.funcs.encode = asset_ref_string_encode_callback;
	m_message.originalPath.funcs.decode = asset_ref_string_decode_callback;
	m_message.originalPath.arg = const_cast<std::string*>(&m_originalPath);

	// 设置 importTime 的回调
	m_message.importTime.funcs.encode = asset_ref_string_encode_callback;
	m_message.importTime.funcs.decode = asset_ref_string_decode_callback;
	m_message.importTime.arg = const_cast<std::string*>(&m_importTime);

	// 设置 engineVersion 的回调
	m_message.engineVersion.funcs.encode = asset_ref_string_encode_callback;
	m_message.engineVersion.funcs.decode = asset_ref_string_decode_callback;
	m_message.engineVersion.arg = const_cast<std::string*>(&m_engineVersion);
}

NS_ASSETPROCESS_END
