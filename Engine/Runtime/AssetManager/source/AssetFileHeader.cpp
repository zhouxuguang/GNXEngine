//
//  AssetFileHeader.cpp
//  GNXEngine
//
//  Created by AI Assistant
//

#include "AssetFileHeader.h"
#include "Runtime/BaseLib/include/HashFunction.h"
#include <iostream>
#include <iomanip>
#include <cstring>

NS_ASSETMANAGER_BEGIN

std::string AssetFileHeader::GetAssetTypeName() const
{
    return AssetManager::GetAssetTypeName(GetAssetType());
}

bool AssetFileHeaderUtil::ReadHeader(const std::string& filepath, AssetFileHeader& header)
{
    std::ifstream stream(filepath, std::ios::binary);
    if (!stream.is_open())
    {
        return false;
    }

    return ReadHeader(stream, header);
}

bool AssetFileHeaderUtil::ReadHeader(std::ifstream& stream, AssetFileHeader& header)
{
    if (!stream.is_open())
    {
        return false;
    }

    stream.read(reinterpret_cast<char*>(&header), sizeof(AssetFileHeader));
    if (stream.fail())
    {
        return false;
    }

    return ValidateHeader(header);
}

bool AssetFileHeaderUtil::ValidateHeader(const AssetFileHeader& header)
{
    // 验证魔数
    if (!header.IsValid())
    {
        return false;
    }

    // 验证版本号
    if (header.version == 0)
    {
        return false;
    }

    // 验证文件头大小
    if (header.headerSize != sizeof(AssetFileHeader))
    {
        return false;
    }

    // 验证数据偏移量
    if (header.dataOffset < sizeof(AssetFileHeader))
    {
        return false;
    }

    return true;
}

bool AssetFileHeaderUtil::WriteHeader(const std::string& filepath, const AssetFileHeader& header)
{
    std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);
    if (!stream.is_open())
    {
        return false;
    }

    return WriteHeader(stream, header);
}

bool AssetFileHeaderUtil::WriteHeader(std::ofstream& stream, const AssetFileHeader& header)
{
    if (!stream.is_open())
    {
        return false;
    }

    stream.write(reinterpret_cast<const char*>(&header), sizeof(AssetFileHeader));
    if (stream.fail())
    {
        return false;
    }

    return true;
}

AssetFileHeader AssetFileHeaderUtil::CreateHeader(
    AssetType type,
    const std::string& name,
    uint64_t hash,
    uint64_t dataSize,
    uint32_t flags)
{
    AssetFileHeader header;
    header.magic = ASSET_MAGIC_NUMBER;
    header.version = CURRENT_ASSET_VERSION;
    header.fileType = static_cast<uint32_t>(type);
    header.headerSize = sizeof(AssetFileHeader);
    header.dataOffset = sizeof(AssetFileHeader);
    header.dataSize = dataSize;
    header.hash = hash;
    header.flags = flags;
    header.reserved1 = 0;
    header.SetName(name);

    return header;
}

void AssetFileHeaderUtil::PrintHeader(const AssetFileHeader& header)
{
    std::cout << "========== Asset File Header ==========" << std::endl;
    std::cout << "Magic Number: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.magic << std::dec << std::endl;
    std::cout << "Version: " << header.version << std::endl;
    std::cout << "Asset Type: " << header.GetAssetTypeName() << std::endl;
    std::cout << "Header Size: " << header.headerSize << " bytes" << std::endl;
    std::cout << "Data Offset: " << header.dataOffset << std::endl;
    std::cout << "Data Size: " << header.dataSize << " bytes" << std::endl;
    std::cout << "Hash: 0x" << std::hex << std::setw(16) << std::setfill('0') << header.hash << std::dec << std::endl;
    std::cout << "Flags: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.flags << std::dec << std::endl;
    std::cout << "  Compressed: " << (header.IsCompressed() ? "Yes" : "No") << std::endl;
    std::cout << "  Encrypted: " << (header.IsEncrypted() ? "Yes" : "No") << std::endl;
    std::cout << "  Streaming: " << (header.IsStreaming() ? "Yes" : "No") << std::endl;
    std::cout << "Asset Name: " << header.GetName() << std::endl;
    std::cout << "=======================================" << std::endl;
}

uint64_t AssetFileHeaderUtil::ComputeHash(const void* data, uint64_t size)
{
    if (data == nullptr || size == 0)
    {
        return 0;
    }

    uint64_t hash = baselib::HashFunction(data, size);

    return hash;
}

uint64_t AssetFileHeaderUtil::ComputeHash(const std::string& filepath, uint64_t offset, uint64_t size)
{
    std::ifstream stream(filepath, std::ios::binary);
    if (!stream.is_open())
    {
        return 0;
    }

    // 定位到指定偏移量
    stream.seekg(offset);
    if (stream.fail())
    {
        return 0;
    }

    // 读取数据
    std::vector<uint8_t> buffer(size);
    stream.read(reinterpret_cast<char*>(buffer.data()), size);
    if (stream.fail())
    {
        return 0;
    }

    return ComputeHash(buffer.data(), buffer.size());
}

NS_ASSETMANAGER_END
