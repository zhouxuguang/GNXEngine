//
//  AssetFileHeaderTest.cpp
//  GNXEngine
//
//  Created by AI Assistant
//  Unit tests for AssetFileHeader functionality
//

#include "Runtime/AssetManager/include/AssetFileHeader.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <fstream>

using namespace AssetManager;

// 测试辅助函数
void TestAssetFileHeaderCreation()
{
    std::cout << "Test: AssetFileHeader Creation..." << std::endl;

    // 创建一个新的文件头
    AssetFileHeader header = AssetFileHeaderUtil::CreateHeader(
        AssetType::Texture,
        "TestTexture",
        0x123456789ABCDEF0ULL,
        1024 * 1024, // 1MB
        AssetFileFlags::COMPRESSED
    );

    // 验证基本字段
    assert(header.magic == ASSET_MAGIC_NUMBER);
    assert(header.version == CURRENT_ASSET_VERSION);
    assert(header.fileType == static_cast<uint32_t>(AssetType::Texture));
    assert(header.headerSize == sizeof(AssetFileHeader));
    assert(header.dataOffset == sizeof(AssetFileHeader));
    assert(header.dataSize == 1024 * 1024);
    assert(header.hash == 0x123456789ABCDEF0ULL);
    assert(header.IsCompressed());
    assert(!header.IsEncrypted());
    assert(!header.IsStreaming());
    assert(header.GetName() == "TestTexture");

    std::cout << "  PASSED" << std::endl;
}

// 测试文件头验证
void TestAssetFileHeaderValidation()
{
    std::cout << "Test: AssetFileHeader Validation..." << std::endl;

    // 创建有效的文件头
    AssetFileHeader validHeader = AssetFileHeaderUtil::CreateHeader(
        AssetType::Mesh,
        "TestMesh",
        0,
        2048
    );

    // 验证有效文件头
    assert(AssetFileHeaderUtil::ValidateHeader(validHeader));
    assert(validHeader.IsValid());

    // 测试无效魔数
    AssetFileHeader invalidHeader = validHeader;
    invalidHeader.magic = 0x12345678;
    assert(!invalidHeader.IsValid());
    assert(!AssetFileHeaderUtil::ValidateHeader(invalidHeader));

    // 测试无效版本
    invalidHeader = validHeader;
    invalidHeader.version = 0;
    assert(!AssetFileHeaderUtil::ValidateHeader(invalidHeader));

    // 测试无效数据偏移
    invalidHeader = validHeader;
    invalidHeader.dataOffset = 64; // 小于头大小
    assert(!AssetFileHeaderUtil::ValidateHeader(invalidHeader));

    std::cout << "  PASSED" << std::endl;
}

// 测试标志位
void TestAssetFileFlags()
{
    std::cout << "Test: AssetFileHeader Flags..." << std::endl;

    AssetFileHeader header = AssetFileHeaderUtil::CreateHeader(
        AssetType::Shader,
        "TestShader",
        0,
        512
    );

    // 测试默认无标志
    assert(!header.IsCompressed());
    assert(!header.IsEncrypted());
    assert(!header.IsStreaming());
    assert(header.flags == AssetFileFlags::NONE);

    // 测试压缩标志
    header.SetCompressed(true);
    assert(header.IsCompressed());
    assert(header.flags & AssetFileFlags::COMPRESSED);

    header.SetCompressed(false);
    assert(!header.IsCompressed());
    assert(!(header.flags & AssetFileFlags::COMPRESSED));

    // 测试加密标志
    header.SetEncrypted(true);
    assert(header.IsEncrypted());
    assert(header.flags & AssetFileFlags::ENCRYPTED);

    header.SetEncrypted(false);
    assert(!header.IsEncrypted());
    assert(!(header.flags & AssetFileFlags::ENCRYPTED));

    // 测试流式标志
    header.SetStreaming(true);
    assert(header.IsStreaming());
    assert(header.flags & AssetFileFlags::STREAMING);

    header.SetStreaming(false);
    assert(!header.IsStreaming());
    assert(!(header.flags & AssetFileFlags::STREAMING));

    // 测试组合标志
    header.SetCompressed(true);
    header.SetEncrypted(true);
    header.SetStreaming(true);
    assert(header.IsCompressed());
    assert(header.IsEncrypted());
    assert(header.IsStreaming());

    std::cout << "  PASSED" << std::endl;
}

// 测试哈希计算
void TestHashComputation()
{
    std::cout << "Test: Hash Computation..." << std::endl;

    // 测试数据
    const char testData1[] = "Hello, World!";
    const char testData2[] = "Hello, World!";
    const char testData3[] = "Different Data";

    // 计算哈希
    uint64_t hash1 = AssetFileHeaderUtil::ComputeHash(testData1, sizeof(testData1));
    uint64_t hash2 = AssetFileHeaderUtil::ComputeHash(testData2, sizeof(testData2));
    uint64_t hash3 = AssetFileHeaderUtil::ComputeHash(testData3, sizeof(testData3));

    // 相同数据应该产生相同的哈希
    assert(hash1 == hash2);

    // 不同数据应该产生不同的哈希
    assert(hash1 != hash3);

    // 测试空数据
    uint64_t emptyHash = AssetFileHeaderUtil::ComputeHash(nullptr, 0);
    assert(emptyHash == 0);

    std::cout << "  PASSED" << std::endl;
}

// 测试文件读写
void TestFileReadWrite()
{
    std::cout << "Test: File Read/Write..." << std::endl;

    const std::string testFile = "test_asset_header.bin";

    // 创建并写入文件头
    AssetFileHeader writeHeader = AssetFileHeaderUtil::CreateHeader(
        AssetType::Material,
        "TestMaterial",
        0xDEADBEEFCAFEBABEULL,
        4096,
        AssetFileFlags::ENCRYPTED | AssetFileFlags::COMPRESSED
    );

    assert(AssetFileHeaderUtil::WriteHeader(testFile, writeHeader));

    // 读取文件头
    AssetFileHeader readHeader;
    assert(AssetFileHeaderUtil::ReadHeader(testFile, readHeader));

    // 验证读取的数据
    assert(readHeader.magic == writeHeader.magic);
    assert(readHeader.version == writeHeader.version);
    assert(readHeader.fileType == writeHeader.fileType);
    assert(readHeader.headerSize == writeHeader.headerSize);
    assert(readHeader.dataOffset == writeHeader.dataOffset);
    assert(readHeader.dataSize == writeHeader.dataSize);
    assert(readHeader.hash == writeHeader.hash);
    assert(readHeader.flags == writeHeader.flags);
    assert(readHeader.GetName() == writeHeader.GetName());

    // 测试流式读写
    {
        std::ofstream outStream(testFile + ".stream", std::ios::binary);
        assert(AssetFileHeaderUtil::WriteHeader(outStream, writeHeader));
        outStream.close();

        std::ifstream inStream(testFile + ".stream", std::ios::binary);
        assert(AssetFileHeaderUtil::ReadHeader(inStream, readHeader));
        inStream.close();
    }

    // 清理测试文件
    std::remove(testFile.c_str());
    std::remove((testFile + ".stream").c_str());

    std::cout << "  PASSED" << std::endl;
}

// 测试资产类型名称
void TestAssetTypeNames()
{
    std::cout << "Test: Asset Type Names..." << std::endl;

    // 测试所有资产类型
    AssetFileHeader header;

    header.fileType = static_cast<uint32_t>(AssetType::Texture);
    assert(header.GetAssetTypeName() == "Texture");

    header.fileType = static_cast<uint32_t>(AssetType::Mesh);
    assert(header.GetAssetTypeName() == "Mesh");

    header.fileType = static_cast<uint32_t>(AssetType::Material);
    assert(header.GetAssetTypeName() == "Material");

    header.fileType = static_cast<uint32_t>(AssetType::Shader);
    assert(header.GetAssetTypeName() == "Shader");

    header.fileType = static_cast<uint32_t>(AssetType::Audio);
    assert(header.GetAssetTypeName() == "Audio");

    header.fileType = static_cast<uint32_t>(AssetType::Animation);
    assert(header.GetAssetTypeName() == "Animation");

    header.fileType = static_cast<uint32_t>(AssetType::Scene);
    assert(header.GetAssetTypeName() == "Scene");

    header.fileType = static_cast<uint32_t>(AssetType::Font);
    assert(header.GetAssetTypeName() == "Font");

    header.fileType = static_cast<uint32_t>(AssetType::Cubemap);
    assert(header.GetAssetTypeName() == "Cubemap");

    header.fileType = static_cast<uint32_t>(AssetType::Lightmap);
    assert(header.GetAssetTypeName() == "Lightmap");

    header.fileType = static_cast<uint32_t>(AssetType::ReflectionProbe);
    assert(header.GetAssetTypeName() == "ReflectionProbe");

    header.fileType = static_cast<uint32_t>(AssetType::Unknown);
    assert(header.GetAssetTypeName() == "Unknown");

    std::cout << "  PASSED" << std::endl;
}

// 测试完整文件读写（包含数据）
void TestCompleteFile()
{
    std::cout << "Test: Complete File with Data..." << std::endl;

    const std::string testFile = "test_complete_asset.bin";

    // 准备测试数据
    std::string testData = "This is test asset data that will be stored after the header.";
    uint64_t hash = AssetFileHeaderUtil::ComputeHash(testData.data(), testData.size());

    // 创建文件头
    AssetFileHeader header = AssetFileHeaderUtil::CreateHeader(
        AssetType::Texture,
        "TestTextureWithData",
        hash,
        testData.size(),
        AssetFileFlags::NONE
    );

    // 写入完整文件（头 + 数据）
    {
        std::ofstream outStream(testFile, std::ios::binary);
        assert(AssetFileHeaderUtil::WriteHeader(outStream, header));
        outStream.write(testData.data(), testData.size());
        outStream.close();
    }

    // 读取并验证
    {
        std::ifstream inStream(testFile, std::ios::binary);
        AssetFileHeader readHeader;
        assert(AssetFileHeaderUtil::ReadHeader(inStream, readHeader));

        // 跳到数据部分
        inStream.seekg(readHeader.dataOffset);
        std::vector<char> readData(readHeader.dataSize);
        inStream.read(readData.data(), readData.size());
        inStream.close();

        // 验证数据
        assert(std::string(readData.data(), readData.size()) == testData);
    }

    // 从文件计算哈希并验证
    uint64_t fileHash = AssetFileHeaderUtil::ComputeHash(
        testFile,
        header.dataOffset,
        header.dataSize
    );
    assert(fileHash == hash);

    // 清理测试文件
    std::remove(testFile.c_str());

    std::cout << "  PASSED" << std::endl;
}

// 主测试函数
int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "AssetFileHeader Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    try
    {
        TestAssetFileHeaderCreation();
        TestAssetFileHeaderValidation();
        TestAssetFileFlags();
        TestHashComputation();
        TestFileReadWrite();
        TestAssetTypeNames();
        TestCompleteFile();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "All Tests PASSED!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Test FAILED with unknown exception" << std::endl;
        return 1;
    }
}
