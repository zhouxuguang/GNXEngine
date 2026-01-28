#include "AssetReference.h"
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

bool AssetReference::LoadFromFile(const std::string& filePath)
{
	// 清空现有数据
	m_originalFileName.clear();
	m_originalPath.clear();
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

NS_ASSETMANAGER_END
