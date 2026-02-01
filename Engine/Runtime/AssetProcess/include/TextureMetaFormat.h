//
//  TextureMetaFormat.h
//  GNXEngine
//
//  纹理 meta 文件格式定义 (YAML)
//

#ifndef GNX_ENGINE_TEXTURE_META_FORMAT_INCLUDE
#define GNX_ENGINE_TEXTURE_META_FORMAT_INCLUDE

#include <string>
#include <vector>
#include <cstdint>
#include "AssetProcessDefine.h"
#include "Runtime/RenderCore/include/TextureFormat.h"

NS_ASSETPROCESS_BEGIN

// ==================== 颜色空间 ====================
enum class TextureColorSpace : uint32_t
{
    Linear = 0,            // 线性颜色空间 (HDR、法线贴图、粗糙度等)
    sRGB = 1,              // sRGB 颜色空间 (颜色贴图、albedo 等)
    Gamma = 2,             // Gamma 校正
};

// ==================== 纹理类型 ====================
enum class TextureType : uint32_t
{
    Texture2D = 0,         // 2D 纹理
    Texture3D = 1,         // 3D 纹理
    Cubemap = 2,           // 立方体贴图
    Texture2DArray = 3,    // 2D 纹理数组
    CubemapArray = 4,      // 立方体贴图数组
};

// ==================== Alpha 处理方式 ====================
enum class AlphaMode : uint32_t
{
    None = 0,              // 无 Alpha
    Straight = 1,          // 普通 Alpha
    Premultiplied = 2,     // 预乘 Alpha
    Mask = 3,              // Alpha 遮罩 (0 或 1)
};

// ==================== Mipmap 生成模式 ====================
enum class MipmapMode : uint32_t
{
    None = 0,              // 不生成 Mipmap
    Auto = 1,              // 自动生成 (使用默认质量)
    Box = 2,               // Box 过滤 (最快)
    Cubic = 3,             // 三次样条插值 (平衡)
    Lanczos = 4,           // Lanczos (最高质量)
};

// ==================== 纹理导入设置 ====================
struct TextureImportSettings
{
    // 是否启用压缩
    bool enableCompression = true;

    // 压缩格式 (平台无关，实际使用时会根据平台自动选择)
    RenderCore::TextureFormat compressFormat = RenderCore::kTexFormatBC7_SRGB;

    // 最大尺寸限制 (0 = 无限制)
    uint32_t maxSize = 0;

    // ==================== Mipmap 设置 ====================

    // Mipmap 生成模式
    MipmapMode mipmapMode = MipmapMode::Auto;

    // 最大 Mipmap 等级 (0 = 全部生成)
    uint32_t maxMipLevel = 0;

    // ==================== 颜色空间 ====================

    // 颜色空间
    TextureColorSpace colorSpace = TextureColorSpace::sRGB;

    // ==================== Alpha 处理 ====================

    // Alpha 处理方式
    AlphaMode alphaMode = AlphaMode::Straight;

    // Alpha 测试阈值 (AlphaMode::Mask 时使用)
    float alphaCutoff = 0.5f;

    // ==================== 纹理类型 ====================

    // 纹理类型
    TextureType textureType = TextureType::Texture2D;

    // ==================== 高级设置 ====================

    // 压缩质量 (0-100, 仅对某些压缩格式有效)
    uint32_t compressQuality = 75;

    // 是否为法线贴图 (特殊处理)
    bool isNormalMap = false;

    // 是否读/写 (默认可读写)
    bool readWrite = false;

    // 是否流式加载
    bool streaming = false;

    // 流式加载的 Mipmap 等级 (如果启用 streaming)
    uint32_t streamingMipLevel = 0;
};

// ==================== 纹理 Meta 数据 ====================
struct TextureMeta
{
    // ==================== 版本信息 ====================
    uint32_t importerVersion = 1;      // 导入器版本
    std::string engineVersion = "1.0.0";  // 引擎版本

    // ==================== 源文件信息 ====================
    std::string sourceFile;            // 源文件名 (不含路径)
    uint64_t sourceFileHash = 0;       // 源文件 hash (用于检测变化)

    // ==================== 纹理信息 ====================
    uint32_t width = 0;                 // 纹理宽度
    uint32_t height = 0;                // 纹理高度
    uint32_t depth = 1;                 // 深度 (3D 纹理)
    uint32_t mipLevels = 1;            // Mipmap 等级数
    uint32_t arrayLayers = 1;           // 数组层数 (纹理数组)
    TextureType textureType = TextureType::Texture2D;  // 纹理类型
    bool hasAlpha = false;              // 是否有 Alpha 通道

    // ==================== 压缩信息 ====================
    bool isCompressed = false;          // 是否已压缩
    uint32_t compressFormat = 0;        // 压缩格式 (数字)
    TextureColorSpace colorSpace = TextureColorSpace::sRGB;  // 颜色空间

    // ==================== 缓存信息 ====================
    uint64_t textureHash = 0;          // 纹理 hash (.texture 文件名)
    std::string textureFile;            // 纹理文件名 (hash.texture)

    // ==================== 导入时间 ====================
    std::string importTime;             // 导入时间 (ISO 8601)

    // ==================== 导入设置 ====================
    TextureImportSettings settings;     // 导入设置
};

// ==================== Meta 文件序列化 ====================
class TextureMetaSerializer
{
public:
    // 序列化到 YAML 文件
    static bool SaveToYAML(const TextureMeta& meta, const std::string& filePath);

    // 从 YAML 文件反序列化
    static bool LoadFromYAML(TextureMeta& meta, const std::string& filePath);

    // 序列化到 YAML 字符串
    static std::string ToYAMLString(const TextureMeta& meta);

    // 从 YAML 字符串反序列化
    static bool FromYAMLString(TextureMeta& meta, const std::string& yaml);
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_TEXTURE_META_FORMAT_INCLUDE
