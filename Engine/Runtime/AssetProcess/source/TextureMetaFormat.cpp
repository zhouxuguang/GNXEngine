//
//  TextureMetaFormat.cpp
//  GNXEngine
//
//  纹理 meta 文件 YAML 序列化实现 (使用 yaml-cpp)
//

#include "TextureMetaFormat.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

// ==================== YAML 节点定义 ====================
namespace YAML
{
    // TextureColorSpace 转换
    template<>
    struct convert<AssetProcess::TextureColorSpace>
    {
        static Node encode(const AssetProcess::TextureColorSpace& rhs)
        {
            return Node(static_cast<uint32_t>(rhs));
        }

        static bool decode(const Node& node, AssetProcess::TextureColorSpace& rhs)
        {
            if (!node.IsScalar())
                return false;
            rhs = static_cast<AssetProcess::TextureColorSpace>(node.as<uint32_t>());
            return true;
        }
    };

    // TextureType 转换
    template<>
    struct convert<AssetProcess::TextureType>
    {
        static Node encode(const AssetProcess::TextureType& rhs)
        {
            return Node(static_cast<uint32_t>(rhs));
        }

        static bool decode(const Node& node, AssetProcess::TextureType& rhs)
        {
            if (!node.IsScalar())
                return false;
            rhs = static_cast<AssetProcess::TextureType>(node.as<uint32_t>());
            return true;
        }
    };

    // AlphaMode 转换
    template<>
    struct convert<AssetProcess::AlphaMode>
    {
        static Node encode(const AssetProcess::AlphaMode& rhs)
        {
            return Node(static_cast<uint32_t>(rhs));
        }

        static bool decode(const Node& node, AssetProcess::AlphaMode& rhs)
        {
            if (!node.IsScalar())
                return false;
            rhs = static_cast<AssetProcess::AlphaMode>(node.as<uint32_t>());
            return true;
        }
    };

    // MipmapMode 转换
    template<>
    struct YAML::convert<AssetProcess::MipmapMode>
    {
        static Node encode(const AssetProcess::MipmapMode& rhs)
        {
            return Node(static_cast<uint32_t>(rhs));
        }

        static bool decode(const Node& node, AssetProcess::MipmapMode& rhs)
        {
            if (!node.IsScalar())
                return false;
            rhs = static_cast<AssetProcess::MipmapMode>(node.as<uint32_t>());
            return true;
        }
    };

    // TextureImportSettings 转换
    template<>
    struct convert<AssetProcess::TextureImportSettings>
    {
        static Node encode(const AssetProcess::TextureImportSettings& rhs)
        {
            Node node;
            node["enableCompression"] = rhs.enableCompression;
            node["compressFormat"] = rhs.compressFormat;
            node["maxSize"] = rhs.maxSize;
            node["mipmapMode"] = rhs.mipmapMode;
            node["maxMipLevel"] = rhs.maxMipLevel;
            node["colorSpace"] = rhs.colorSpace;
            node["alphaMode"] = rhs.alphaMode;
            node["alphaCutoff"] = rhs.alphaCutoff;
            node["textureType"] = rhs.textureType;
            node["compressQuality"] = rhs.compressQuality;
            node["isNormalMap"] = rhs.isNormalMap;
            node["readWrite"] = rhs.readWrite;
            node["streaming"] = rhs.streaming;
            node["streamingMipLevel"] = rhs.streamingMipLevel;
            return node;
        }

        static bool decode(const Node& node, AssetProcess::TextureImportSettings& rhs)
        {
            if (!node.IsMap())
                return false;

            if (node["enableCompression"])
                rhs.enableCompression = node["enableCompression"].as<bool>(true);
            if (node["compressFormat"])
                rhs.compressFormat = node["compressFormat"].as<RenderCore::TextureFormat>(RenderCore::kTexFormatBC7_SRGB);
            if (node["maxSize"])
                rhs.maxSize = node["maxSize"].as<uint32_t>(0);
            if (node["mipmapMode"])
                rhs.mipmapMode = node["mipmapMode"].as<AssetProcess::MipmapMode>(AssetProcess::MipmapMode::Auto);
            if (node["maxMipLevel"])
                rhs.maxMipLevel = node["maxMipLevel"].as<uint32_t>(0);
            if (node["colorSpace"])
                rhs.colorSpace = node["colorSpace"].as<AssetProcess::TextureColorSpace>(AssetProcess::TextureColorSpace::sRGB);
            if (node["alphaMode"])
                rhs.alphaMode = node["alphaMode"].as<AssetProcess::AlphaMode>(AssetProcess::AlphaMode::Straight);
            if (node["alphaCutoff"])
                rhs.alphaCutoff = node["alphaCutoff"].as<float>(0.5f);
            if (node["textureType"])
                rhs.textureType = node["textureType"].as<AssetProcess::TextureType>(AssetProcess::TextureType::Texture2D);
            if (node["compressQuality"])
                rhs.compressQuality = node["compressQuality"].as<uint32_t>(75);
            if (node["isNormalMap"])
                rhs.isNormalMap = node["isNormalMap"].as<bool>(false);
            if (node["readWrite"])
                rhs.readWrite = node["readWrite"].as<bool>(false);
            if (node["streaming"])
                rhs.streaming = node["streaming"].as<bool>(false);
            if (node["streamingMipLevel"])
                rhs.streamingMipLevel = node["streamingMipLevel"].as<uint32_t>(0);

            return true;
        }
    };

    // TextureMeta 转换
    template<>
    struct convert<AssetProcess::TextureMeta>
    {
        static Node encode(const AssetProcess::TextureMeta& rhs)
        {
            Node node;

            // 版本信息
            node["importerVersion"] = rhs.importerVersion;
            node["engineVersion"] = rhs.engineVersion;

            // 源文件信息
            node["sourceFile"] = rhs.sourceFile;
            node["sourceFileHash"] = rhs.sourceFileHash;

            // 纹理信息
            node["width"] = rhs.width;
            node["height"] = rhs.height;
            node["depth"] = rhs.depth;
            node["mipLevels"] = rhs.mipLevels;
            node["arrayLayers"] = rhs.arrayLayers;
            node["textureType"] = rhs.textureType;
            node["hasAlpha"] = rhs.hasAlpha;

            // 压缩信息
            node["isCompressed"] = rhs.isCompressed;
            node["compressFormat"] = rhs.compressFormat;
            node["colorSpace"] = rhs.colorSpace;

            // 缓存信息
            node["textureHash"] = rhs.textureHash;
            node["textureFile"] = rhs.textureFile;

            // 导入时间
            node["importTime"] = rhs.importTime;

            // 导入设置
            node["settings"] = rhs.settings;

            return node;
        }

        static bool decode(const Node& node, AssetProcess::TextureMeta& rhs)
        {
            if (!node.IsMap())
                return false;

            // 版本信息
            if (node["importerVersion"])
                rhs.importerVersion = node["importerVersion"].as<uint32_t>(1);
            if (node["engineVersion"])
                rhs.engineVersion = node["engineVersion"].as<std::string>("1.0.0");

            // 源文件信息
            if (node["sourceFile"])
                rhs.sourceFile = node["sourceFile"].as<std::string>();
            if (node["sourceFileHash"])
                rhs.sourceFileHash = node["sourceFileHash"].as<uint64_t>(0);

            // 纹理信息
            if (node["width"])
                rhs.width = node["width"].as<uint32_t>(0);
            if (node["height"])
                rhs.height = node["height"].as<uint32_t>(0);
            if (node["depth"])
                rhs.depth = node["depth"].as<uint32_t>(1);
            if (node["mipLevels"])
                rhs.mipLevels = node["mipLevels"].as<uint32_t>(1);
            if (node["arrayLayers"])
                rhs.arrayLayers = node["arrayLayers"].as<uint32_t>(1);
            if (node["textureType"])
                rhs.textureType = node["textureType"].as<AssetProcess::TextureType>(AssetProcess::TextureType::Texture2D);
            if (node["hasAlpha"])
                rhs.hasAlpha = node["hasAlpha"].as<bool>(false);

            // 压缩信息
            if (node["isCompressed"])
                rhs.isCompressed = node["isCompressed"].as<bool>(false);
            if (node["compressFormat"])
                rhs.compressFormat = node["compressFormat"].as<uint32_t>(0);
            if (node["colorSpace"])
                rhs.colorSpace = node["colorSpace"].as<AssetProcess::TextureColorSpace>(AssetProcess::TextureColorSpace::sRGB);

            // 缓存信息
            if (node["textureHash"])
                rhs.textureHash = node["textureHash"].as<uint64_t>(0);
            if (node["textureFile"])
                rhs.textureFile = node["textureFile"].as<std::string>();

            // 导入时间
            if (node["importTime"])
                rhs.importTime = node["importTime"].as<std::string>();

            // 导入设置
            if (node["settings"])
                rhs.settings = node["settings"].as<AssetProcess::TextureImportSettings>();

            return true;
        }
    };
}

NS_ASSETPROCESS_BEGIN

// ==================== 序列化到 YAML 字符串 ====================

std::string TextureMetaSerializer::ToYAMLString(const TextureMeta& meta)
{
    try
    {
        YAML::Emitter emitter;

        // 设置输出格式
        emitter << YAML::BeginMap;

        // 版本信息
        emitter << YAML::Key << "importerVersion";
        emitter << YAML::Value << meta.importerVersion;

        emitter << YAML::Key << "engineVersion";
        emitter << YAML::Value << meta.engineVersion;

        // 源文件信息
        emitter << YAML::Newline << YAML::Newline;
        emitter << YAML::Key << "sourceFile";
        emitter << YAML::Value << meta.sourceFile;

        emitter << YAML::Key << "sourceFileHash";
        emitter << YAML::Value << meta.sourceFileHash;

        // 纹理信息
        emitter << YAML::Newline << YAML::Newline;
        emitter << YAML::Key << "width";
        emitter << YAML::Value << meta.width;

        emitter << YAML::Key << "height";
        emitter << YAML::Value << meta.height;

        emitter << YAML::Key << "depth";
        emitter << YAML::Value << meta.depth;

        emitter << YAML::Key << "mipLevels";
        emitter << YAML::Value << meta.mipLevels;

        emitter << YAML::Key << "arrayLayers";
        emitter << YAML::Value << meta.arrayLayers;

        emitter << YAML::Key << "textureType";
        emitter << YAML::Value << static_cast<uint32_t>(meta.textureType);

        emitter << YAML::Key << "hasAlpha";
        emitter << YAML::Value << meta.hasAlpha;

        // 压缩信息
        emitter << YAML::Newline << YAML::Newline;
        emitter << YAML::Key << "isCompressed";
        emitter << YAML::Value << meta.isCompressed;

        emitter << YAML::Key << "compressFormat";
        emitter << YAML::Value << meta.compressFormat;

        emitter << YAML::Key << "colorSpace";
        emitter << YAML::Value << static_cast<uint32_t>(meta.colorSpace);

        // 缓存信息
        emitter << YAML::Newline << YAML::Newline;
        emitter << YAML::Key << "textureHash";
        emitter << YAML::Value << meta.textureHash;

        emitter << YAML::Key << "textureFile";
        emitter << YAML::Value << meta.textureFile;

        // 导入时间
        emitter << YAML::Newline << YAML::Newline;
        emitter << YAML::Key << "importTime";
        emitter << YAML::Value << meta.importTime;

        // 导入设置
        emitter << YAML::Newline << YAML::Newline;
        emitter << YAML::Key << "settings";

        // 设置子节点
        emitter << YAML::BeginMap;

        // 基本设置
        emitter << YAML::Key << "enableCompression";
        emitter << YAML::Value << meta.settings.enableCompression;

        emitter << YAML::Key << "compressFormat";
        emitter << YAML::Value << static_cast<uint32_t>(meta.settings.compressFormat);

        emitter << YAML::Key << "maxSize";
        emitter << YAML::Value << meta.settings.maxSize;

        // Mipmap 设置
        emitter << YAML::Newline;
        emitter << YAML::Key << "mipmapMode";
        emitter << YAML::Value << static_cast<uint32_t>(meta.settings.mipmapMode);

        emitter << YAML::Key << "maxMipLevel";
        emitter << YAML::Value << meta.settings.maxMipLevel;

        // 颜色空间
        emitter << YAML::Newline;
        emitter << YAML::Key << "colorSpace";
        emitter << YAML::Value << static_cast<uint32_t>(meta.settings.colorSpace);

        // Alpha 处理
        emitter << YAML::Newline;
        emitter << YAML::Key << "alphaMode";
        emitter << YAML::Value << static_cast<uint32_t>(meta.settings.alphaMode);

        emitter << YAML::Key << "alphaCutoff";
        emitter << YAML::Value << meta.settings.alphaCutoff;

        // 纹理类型
        emitter << YAML::Newline;
        emitter << YAML::Key << "textureType";
        emitter << YAML::Value << static_cast<uint32_t>(meta.settings.textureType);

        // 高级设置
        emitter << YAML::Newline;
        emitter << YAML::Key << "compressQuality";
        emitter << YAML::Value << meta.settings.compressQuality;

        emitter << YAML::Key << "isNormalMap";
        emitter << YAML::Value << meta.settings.isNormalMap;

        emitter << YAML::Key << "readWrite";
        emitter << YAML::Value << meta.settings.readWrite;

        emitter << YAML::Key << "streaming";
        emitter << YAML::Value << meta.settings.streaming;

        emitter << YAML::Key << "streamingMipLevel";
        emitter << YAML::Value << meta.settings.streamingMipLevel;

        emitter << YAML::EndMap;

        emitter << YAML::EndMap;

        return emitter.c_str();
    }
    catch (const YAML::Exception& e)
    {
        LOG_ERROR("Failed to serialize TextureMeta to YAML: %s", e.what());
        return "";
    }
}

// ==================== 从 YAML 字符串反序列化 ====================

bool TextureMetaSerializer::FromYAMLString(TextureMeta& meta, const std::string& yaml)
{
    try
    {
        YAML::Node node = YAML::Load(yaml);

        if (!node.IsMap())
        {
            LOG_ERROR("Invalid YAML format: expected map");
            return false;
        }

        // 使用自定义转换器反序列化
        meta = node.as<TextureMeta>();

        LOG_INFO("Successfully deserialized TextureMeta from YAML");
        return true;
    }
    catch (const YAML::Exception& e)
    {
        LOG_ERROR("Failed to parse YAML: %s", e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to deserialize TextureMeta: %s", e.what());
        return false;
    }
}

// ==================== 序列化到 YAML 文件 ====================

bool TextureMetaSerializer::SaveToYAML(const TextureMeta& meta, const std::string& filePath)
{
    try
    {
        // 序列化为 YAML 字符串
        std::string yaml = ToYAMLString(meta);
        if (yaml.empty())
        {
            LOG_ERROR("Failed to serialize TextureMeta to YAML string");
            return false;
        }

        // 保存文件
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open meta file for writing: %s", filePath.c_str());
            return false;
        }

        file << yaml;
        file.close();

        LOG_INFO("Saved texture meta file: %s", filePath.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to save meta file: %s, error: %s", filePath.c_str(), e.what());
        return false;
    }
}

// ==================== 从 YAML 文件反序列化 ====================

bool TextureMetaSerializer::LoadFromYAML(TextureMeta& meta, const std::string& filePath)
{
    try
    {
        // 检查目标文件是否存在
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open meta file for reading: %s", filePath.c_str());
            return false;
        }

        // 读取文件内容
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string yaml = buffer.str();

        // 反序列化
        return FromYAMLString(meta, yaml);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to load meta file: %s, error: %s", filePath.c_str(), e.what());
        return false;
    }
}

NS_ASSETPROCESS_END
