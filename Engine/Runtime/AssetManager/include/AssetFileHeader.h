#ifndef GNXENGINE_INCLUDE_ASSET_FILE_HEADER_INCLS
#define GNXENGINE_INCLUDE_ASSET_FILE_HEADER_INCLS

#include <cstdint>
#include <string>
#include <fstream>
#include <algorithm>
#include <cstring>
#include "AssetType.h"

NS_ASSETMANAGER_BEGIN

/**
 * 文件头魔数，用于快速验证文件类型
 * "GNXA" = GNX Asset
 */
constexpr uint32_t ASSET_MAGIC_NUMBER = 0x474E5841;

/**
 * 文件格式版本
 */
constexpr uint32_t CURRENT_ASSET_VERSION = 1;

/**
 * 文件头标志位
 */
namespace AssetFileFlags
{
    constexpr uint32_t NONE = 0;          // 无特殊标志
    constexpr uint32_t COMPRESSED = 1;   // 数据已压缩
    constexpr uint32_t ENCRYPTED = 2;     // 数据已加密
    constexpr uint32_t STREAMING = 4;    // 支持流式加载
}

/**
 * 文件头标志位工具函数
 */
namespace AssetFileFlagsUtil
{
    inline bool IsCompressed(uint32_t flags)
    {
        return (flags & AssetFileFlags::COMPRESSED) != 0;
    }
    inline bool IsEncrypted(uint32_t flags)
    {
        return (flags & AssetFileFlags::ENCRYPTED) != 0;
    }
    inline bool IsStreaming(uint32_t flags)
    {
        return (flags & AssetFileFlags::STREAMING) != 0;
    }
}

/**
 * 统一的资产文件头（固定128字节，方便快速读取）
 */
struct ASSET_MANAGER_API AssetFileHeader 
{
    uint32_t magic;           // 魔数 (0x474E5841 = "GNXA")
    uint32_t version;         // 文件格式版本
    uint32_t fileType;        // 资产类型 (AssetType)
    uint32_t headerSize;      // 文件头总大小（本结构体大小）
    uint64_t dataOffset;      // 实际数据开始的偏移量（通常是128）
    uint64_t dataSize;        // 数据部分的大小
    uint64_t hash;            // 内容哈希（用于快速查找和缓存）
    uint32_t flags;           // 标志位（压缩、加密等）
    uint32_t reserved1;       // 保留字段
    char name[76];            // 资产名称（可选，便于调试）

    /**
     * 构造函数，初始化为默认值
     */
    AssetFileHeader()
        : magic(0)
        , version(0)
        , fileType(0)
        , headerSize(0)
        , dataOffset(0)
        , dataSize(0)
        , hash(0)
        , flags(0)
        , reserved1(0)
    {
        memset(name, 0, sizeof(name));
    }

    /**
     * 验证魔数
     */
    bool IsValid() const
    {
        return magic == ASSET_MAGIC_NUMBER;
    }

    /**
     * 获取资产类型
     */
    AssetType GetAssetType() const
    {
        return static_cast<AssetType>(fileType);
    }

    /**
     * 获取资产类型名称
     */
    std::string GetAssetTypeName() const;

    /**
     * 获取资产名称
     */
    std::string GetName() const
    {
        return std::string(name);
    }

    /**
     * 设置资产名称
     */
    void SetName(const std::string& assetName)
    {
        strncpy_s(name, assetName.c_str(), sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }

    /**
     * 检查是否已压缩
     */
    bool IsCompressed() const
    {
        return AssetFileFlagsUtil::IsCompressed(flags);
    }

    /**
     * 检查是否已加密
     */
    bool IsEncrypted() const
    {
        return AssetFileFlagsUtil::IsEncrypted(flags);
    }

    /**
     * 检查是否支持流式加载
     */
    bool IsStreaming() const
    {
        return AssetFileFlagsUtil::IsStreaming(flags);
    }

    /**
     * 设置压缩标志
     */
    void SetCompressed(bool compressed)
    {
        if (compressed)
        {
            flags |= AssetFileFlags::COMPRESSED;
        }
        else
        {
            flags &= ~AssetFileFlags::COMPRESSED;
        }
    }

    /**
     * 设置加密标志
     */
    void SetEncrypted(bool encrypted)
    {
        if (encrypted)
        {
            flags |= AssetFileFlags::ENCRYPTED;
        }
        else
        {
            flags &= ~AssetFileFlags::ENCRYPTED;
        }
    }

    /**
     * 设置流式加载标志
     */
    void SetStreaming(bool streaming)
    {
        if (streaming)
        {
            flags |= AssetFileFlags::STREAMING;
        }
        else
        {
            flags &= ~AssetFileFlags::STREAMING;
        }
    }

    /**
     * 获取文件头总大小
     */
    static constexpr uint32_t GetHeaderSize()
    {
        return sizeof(AssetFileHeader);
    }
};

static_assert(sizeof(AssetFileHeader) == 128, "AssetFileHeader must be exactly 128 bytes");

/**
 * 文件头操作工具类
 */
class ASSET_MANAGER_API AssetFileHeaderUtil 
{
public:
    /**
     * 从文件读取文件头
     * @param filepath 文件路径
     * @param header 输出参数，存储读取到的文件头
     * @return 是否成功读取
     */
    static bool ReadHeader(const std::string& filepath, AssetFileHeader& header);

    /**
     * 从输入流读取文件头
     * @param stream 输入流
     * @param header 输出参数，存储读取到的文件头
     * @return 是否成功读取
     */
    static bool ReadHeader(std::ifstream& stream, AssetFileHeader& header);

    /**
     * 验证文件头
     * @param header 文件头
     * @return 是否有效
     */
    static bool ValidateHeader(const AssetFileHeader& header);

    /**
     * 写入文件头到文件（覆盖模式）
     * @param filepath 文件路径
     * @param header 文件头
     * @return 是否成功写入
     */
    static bool WriteHeader(const std::string& filepath, const AssetFileHeader& header);

    /**
     * 写入文件头到输出流
     * @param stream 输出流
     * @param header 文件头
     * @return 是否成功写入
     */
    static bool WriteHeader(std::ofstream& stream, const AssetFileHeader& header);

    /**
     * 创建新的文件头
     * @param type 资产类型
     * @param name 资产名称
     * @param hash 内容哈希
     * @param dataSize 数据大小
     * @param flags 标志位
     * @return 创建的文件头
     */
    static AssetFileHeader CreateHeader(
        AssetType type,
        const std::string& name,
        uint64_t hash,
        uint64_t dataSize,
        uint32_t flags = AssetFileFlags::NONE
    );

    /**
     * 打印文件头信息（调试用）
     * @param header 文件头
     */
    static void PrintHeader(const AssetFileHeader& header);

    /**
     * 计算文件哈希（简单的实现，后续可以替换为更强的哈希算法）
     * @param data 数据指针
     * @param size 数据大小
     * @return 哈希值
     */
    static uint64_t ComputeHash(const void* data, uint64_t size);

    /**
     * 计算文件哈希（从文件）
     * @param filepath 文件路径
     * @param offset 起始偏移量
     * @param size 数据大小
     * @return 哈希值
     */
    static uint64_t ComputeHash(const std::string& filepath, uint64_t offset, uint64_t size);
};

NS_ASSETMANAGER_END

#endif // !GNXENGINE_INCLUDE_ASSET_FILE_HEADER_INCLS
