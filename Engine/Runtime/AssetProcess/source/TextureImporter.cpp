#include "TextureImporter.h"
#include "AssetReference.h"
#include "TextureMetaFormat.h"
#include "Runtime/AssetManager/include/TextureMessage.pb.h"
#include "Runtime/AssetManager/include/TextureMessageUtil.h"
#include "ktx.h"
#include "Runtime/ImageCodec/include/ImageUtil.h"
#include "TextureProcess/stb_image_resize2.h"
#include "DXTCompressor.h"
#include "PVRCompressor.h"
#include "ASTCCompressor.h"
#include "AssetFileHeader.h"
#include "Runtime/BaseLib/include/AlignedMalloc.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include "IBLBaker/PBRBase.h"

namespace fs = std::filesystem;

NS_ASSETPROCESS_BEGIN

// 导入器版本（用于检测设置格式变化）
const uint32_t IMPORTER_VERSION = 1;

struct KTXFormat
{
    uint32_t glInternalformat;
    uint32_t vkFormat;
    stbir_pixel_layout stbLayout;
    stbir_datatype stbDatatype;
    stbir_edge stbEdge;
    stbir_filter stbFilter;
};

static const uint32_t VK_FORMAT_R16G16_UINT = 81;
static const uint32_t VK_FORMAT_R16G16_SINT = 82;
static const uint32_t VK_FORMAT_R16G16_SFLOAT = 83;
static const uint32_t VK_FORMAT_R32G32_UINT = 101;
static const uint32_t VK_FORMAT_R32G32_SINT = 102;
static const uint32_t VK_FORMAT_R32G32_SFLOAT = 103;
static const uint32_t VK_FORMAT_R32G32B32A32_SFLOAT = 109;
static const uint32_t VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131;
static const uint32_t VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132;
static const uint32_t VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133;
static const uint32_t VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134;
static const uint32_t VK_FORMAT_BC2_UNORM_BLOCK = 135;
static const uint32_t VK_FORMAT_BC2_SRGB_BLOCK = 136;
static const uint32_t VK_FORMAT_BC3_UNORM_BLOCK = 137;
static const uint32_t VK_FORMAT_BC3_SRGB_BLOCK = 138;
static const uint32_t VK_FORMAT_BC4_UNORM_BLOCK = 139;
static const uint32_t VK_FORMAT_BC4_SNORM_BLOCK = 140;
static const uint32_t VK_FORMAT_BC5_UNORM_BLOCK = 141;
static const uint32_t VK_FORMAT_BC5_SNORM_BLOCK = 142;
static const uint32_t VK_FORMAT_BC6H_UFLOAT_BLOCK = 143;
static const uint32_t VK_FORMAT_BC6H_SFLOAT_BLOCK = 144;
static const uint32_t VK_FORMAT_BC7_UNORM_BLOCK = 145;
static const uint32_t VK_FORMAT_BC7_SRGB_BLOCK = 146;

static const uint32_t VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000;
static const uint32_t VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001;
static const uint32_t VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002;
static const uint32_t VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003;
static const uint32_t VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004;
static const uint32_t VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005;
static const uint32_t VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006;
static const uint32_t VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007;


#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                      0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                     0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                     0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                     0x83F3

#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT                     0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT               0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT               0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT               0x8C4F

#define GL_COMPRESSED_RED_RGTC1                              0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1                       0x8DBC
#define GL_COMPRESSED_RG_RGTC2                               0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2                        0x8DBE

#define GL_COMPRESSED_RGBA_BPTC_UNORM                        0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM                  0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT                  0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT                0x8E8F

#define GL_RGBA32F                        0x8814
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230

#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG                   0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG                   0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG                  0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG                  0x8C03
#define GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT                  0x8A54
#define GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT                  0x8A55
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT            0x8A56
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT            0x8A57


// 创建导入的格式信息，根据导入设置和格式信息自动推导
KTXFormat CreateKTXFormat(uint32_t imageFormat)
{
    KTXFormat ktxFormat = {};
    switch (imageFormat)
    {
    case imagecodec::FORMAT_GRAY8:
        ktxFormat.glInternalformat = GL_COMPRESSED_RED_RGTC1;
        ktxFormat.vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
        ktxFormat.stbLayout = STBIR_1CHANNEL;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_GRAY8_ALPHA8:
        ktxFormat.glInternalformat = GL_COMPRESSED_RG_RGTC2;
        ktxFormat.vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
        ktxFormat.stbLayout = STBIR_2CHANNEL;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_RGBA8:
        ktxFormat.glInternalformat = GL_COMPRESSED_RGBA_BPTC_UNORM;
        ktxFormat.vkFormat = VK_FORMAT_BC7_UNORM_BLOCK;
        ktxFormat.stbLayout = STBIR_RGBA;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_RGB8:
        ktxFormat.glInternalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        ktxFormat.vkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        ktxFormat.stbLayout = STBIR_RGB;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_SRGB8_ALPHA8:
        ktxFormat.glInternalformat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
        ktxFormat.vkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
        ktxFormat.stbLayout = STBIR_RGBA;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8_SRGB_ALPHA;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_SRGB8:
        ktxFormat.glInternalformat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
        ktxFormat.vkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        ktxFormat.stbLayout = STBIR_RGB;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8_SRGB;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_RGBA32Float:
        ktxFormat.glInternalformat = GL_RGBA32F;
        ktxFormat.vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        ktxFormat.stbLayout = STBIR_RGBA;
        ktxFormat.stbDatatype = STBIR_TYPE_FLOAT;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_RGB32Float:
        ktxFormat.glInternalformat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
        ktxFormat.vkFormat = VK_FORMAT_BC6H_UFLOAT_BLOCK;
        ktxFormat.stbLayout = STBIR_RGB;
        ktxFormat.stbDatatype = STBIR_TYPE_FLOAT;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_RG16Float:
        ktxFormat.glInternalformat = GL_RG16F;
        ktxFormat.vkFormat = VK_FORMAT_R16G16_SFLOAT;
        ktxFormat.stbLayout = STBIR_2CHANNEL;
        ktxFormat.stbDatatype = STBIR_TYPE_HALF_FLOAT;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    case imagecodec::FORMAT_RG32Float:
        ktxFormat.glInternalformat = GL_RG32F;
        ktxFormat.vkFormat = VK_FORMAT_R32G32_SFLOAT;
        ktxFormat.stbLayout = STBIR_2CHANNEL;
        ktxFormat.stbDatatype = STBIR_TYPE_FLOAT;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
        break;
    }

    return ktxFormat;
}

TextureImporter::TextureImporter() : mSourceFileHash(0), mTextureHash(0)
{
	// 初始化 meta 信息
	mMeta.importerVersion = IMPORTER_VERSION;
	mMeta.engineVersion = "1.0.0";
}

TextureImporter::~TextureImporter()
{
}

// ==================== 主要导入接口 ====================

bool TextureImporter::Import(const std::string& sourceFilePath, const std::string& currentDir)
{
    //GenerateBRDFLUT_Texture("", 512, 1024);
	mSourceFilePath = sourceFilePath;
	mCurrentDir = currentDir;

	// 1. 获取源文件名和扩展名
	fs::path sourcePath(sourceFilePath);
	std::string fileName = sourcePath.filename().string();
	std::string targetFilePath = (fs::path(currentDir) / fileName).string();

	// 2. 确保目标目录存在
	fs::path targetDir(currentDir);
	if (!fs::exists(targetDir))
	{
		fs::create_directories(targetDir);
	}

	// 3. 如果目标文件不存在，或与源文件不同，则拷贝
	bool needCopy = false;
	if (!fs::exists(targetFilePath))
	{
		needCopy = true;  // 首次拷贝
	}
	else
	{
		// 比较文件修改时间
		auto sourceTime = fs::last_write_time(sourcePath);
		auto targetTime = fs::last_write_time(targetFilePath);
		if (sourceTime > targetTime)
		{
			needCopy = true;  // 源文件更新，需要重新拷贝
		}
	}

	// 4. 拷贝文件到目标目录
	if (needCopy)
	{
        fs::copy_file(sourcePath, targetFilePath, fs::copy_options::overwrite_existing);
	}

	// 5. 更新源文件路径为拷贝后的路径
	mSourceFilePath = targetFilePath;

	// 6. 读取原始文件（从拷贝后的路径）
	std::vector<uint8_t> sourceData = baselib::FileUtil::ReadBinaryFile(targetFilePath);
	if (sourceData.empty())
	{
		return false;
	}

	// 7. 计算源文件 hash
    mSourceFileHash = baselib::HashFunction(sourceData.data(), sourceData.size());

	// 8. 加载 .meta 文件（如果存在）
	std::string metaFilePath = GetMetaFilePath(targetFilePath);
	bool metaExists = fs::exists(metaFilePath);
	
	if (metaExists)
	{
		if (!LoadMetaFile(metaFilePath))
		{
			// meta 文件损坏，需要重新导入
			metaExists = false;
		}
	}

	// 9. 检查是否需要重新导入
	bool needReimport = false;
	if (!metaExists)
	{
		needReimport = true;  // 首次导入
	}

	// 10. 解码图像
	imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
	bool result = imagecodec::ImageDecoder::DecodeMemory(sourceData.data(), sourceData.size(), image.get());
	if (!result)
	{
		return false;
	}

	// 11. 更新 meta 信息
	mMeta.sourceFile = fs::path(targetFilePath).filename().string();
	mMeta.sourceFileHash = mSourceFileHash;
	mMeta.width = image->GetWidth();
	mMeta.height = image->GetHeight();
	mMeta.depth = 1;
	mMeta.arrayLayers = 1;
	mMeta.textureType = TextureType::Texture2D;
	//mMeta.hasAlpha = imagecodec::hasAlphaChannel(image->GetFormat());

	// 12. 应用默认设置（如果是首次导入）
	if (!metaExists)
	{
		std::string fileNameStr = fs::path(targetFilePath).filename().string();
		ApplyDefaultSettings(fileNameStr, image->GetFormat());
	}

	// 13. 压缩纹理
    std::string textureFilePath = GetTextureFilePath(mSourceFileHash, currentDir);
	if (!CompressTexture(image, textureFilePath))
	{
		return false;
	}

	// 14. 保存导入时间
	auto now = std::time(nullptr);
	auto tm = *std::localtime(&now);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
	mMeta.importTime = oss.str();

	// 15. 保存 .meta 文件
	if (!SaveMetaFile(metaFilePath))
	{
		return false;
	}

	// 16. 生成缩略图
	// currentDir 是 Assets 目录，项目根目录是 Assets 的父目录
	fs::path assetsPath(currentDir);
	fs::path projectRoot = assetsPath.parent_path();
	if (!GenerateThumbnail(image, mSourceFileHash, projectRoot.string()))
	{
		LOG_WARN("Failed to generate thumbnail for: %s", targetFilePath.c_str());
	}

	return true;
}

bool TextureImporter::NeedsReimport(const std::string& sourceFilePath, const std::string& projectRootPath)
{
	std::string metaFilePath = GetMetaFilePath(sourceFilePath);
	
	// 如果 meta 文件不存在，需要导入
	if (!fs::exists(metaFilePath))
	{
		return true;
	}

	// 加载 meta 文件
	TextureMeta meta;
	if (!TextureMetaSerializer::LoadFromYAML(meta, metaFilePath))
	{
		return true;  // meta 文件损坏
	}

	// 检查源文件 hash 是否变化
	uint64_t currentHash = CalculateSourceFileHash(sourceFilePath);
	if (currentHash != meta.sourceFileHash)
	{
		return true;
	}

	return false;
}

void TextureImporter::RemoveImportedTexture(const std::string& sourceFilePath, const std::string& projectRootPath)
{
	// 加载 meta 文件获取 texture hash
	std::string metaFilePath = GetMetaFilePath(sourceFilePath);
	TextureMeta meta;

	if (TextureMetaSerializer::LoadFromYAML(meta, metaFilePath))
	{
		// 删除 .texture 文件
		std::string textureFilePath = GetTextureFilePath(meta.textureHash, projectRootPath);
		fs::remove(textureFilePath);
	}

	// 删除 .meta 文件
	fs::remove(metaFilePath);
}

// ==================== 获取信息 ====================

uint64_t TextureImporter::GetSourceFileHash() const
{
    return mSourceFileHash;
}

uint64_t TextureImporter::GetTextureHash() const
{
    return mTextureHash;
}

const TextureMeta& TextureImporter::GetTextureMeta() const
{
    return mMeta;
}

TextureMeta& TextureImporter::GetTextureMeta()
{
    return mMeta;
}

// ==================== 导入流程内部方法 ====================

uint64_t TextureImporter::CalculateSourceFileHash(const std::string& filePath)
{
	std::vector<uint8_t> data = baselib::FileUtil::ReadBinaryFile(filePath);
	if (data.empty())
	{
		return 0;
	}

    return baselib::HashFunction(data.data(), data.size());
}

bool TextureImporter::LoadMetaFile(const std::string& metaFilePath)
{
	// 使用 YAML 序列化器加载 meta 文件
	return TextureMetaSerializer::LoadFromYAML(mMeta, metaFilePath);
}

bool TextureImporter::SaveMetaFile(const std::string& metaFilePath)
{
	// 使用 YAML 序列化器保存 meta 文件
	return TextureMetaSerializer::SaveToYAML(mMeta, metaFilePath);
}

void TextureImporter::ApplyDefaultSettings(const std::string& fileName, imagecodec::ImagePixelFormat format)
{
	// 自动检测纹理类型
	std::string lowerFileName = fileName;
	std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

	// 检测颜色空间
	if (lowerFileName.find("normal") != std::string::npos ||
	    lowerFileName.find("_n.") != std::string::npos ||
	    lowerFileName.find("-n.") != std::string::npos)
	{
		// 法线贴图
		mMeta.settings.isNormalMap = true;
		mMeta.settings.colorSpace = TextureColorSpace::Linear;
		mMeta.settings.alphaMode = AlphaMode::None;
	}
	else if (lowerFileName.find("albedo") != std::string::npos ||
	         lowerFileName.find("diffuse") != std::string::npos ||
	         lowerFileName.find("color") != std::string::npos)
	{
		// 颜色贴图
		mMeta.settings.colorSpace = TextureColorSpace::sRGB;
		mMeta.settings.isNormalMap = false;
	}
	else
	{
		// 其他贴图（粗糙度、金属度等）
		mMeta.settings.colorSpace = TextureColorSpace::Linear;
		mMeta.settings.isNormalMap = false;
	}

	// 默认设置
	mMeta.settings.enableCompression = true;
	mMeta.settings.mipmapMode = MipmapMode::Auto;
	mMeta.settings.compressQuality = 75;

	// 设置压缩信息
	mMeta.isCompressed = mMeta.settings.enableCompression;
	mMeta.colorSpace = mMeta.settings.colorSpace;
	mMeta.compressFormat = static_cast<uint32_t>(mMeta.settings.compressFormat);
}

bool TextureImporter::CompressTexture(imagecodec::VImagePtr image, const std::string& textureFilePath)
{
	// 生成 KTX 数据
	std::vector<uint8_t> ktxData = GenerateKTXData(image, mMeta.settings);
	if (ktxData.empty())
	{
		return false;
	}

	// 计算纹理 hash（基于 KTX 数据）
    mTextureHash = baselib::HashFunction(ktxData.data(), ktxData.size());

	// 保存 .texture 文件
	return SaveTextureFile(textureFilePath, ktxData, fs::path(mSourceFilePath).filename().string());
}

static void CompressTextureInner(const uint8_t* imageData, uint32_t width, uint32_t height, uint8_t* pDest, uint32_t vkFormat, uint32_t imageBytes)
{
    if (vkFormat == VK_FORMAT_BC1_RGB_UNORM_BLOCK || vkFormat == VK_FORMAT_BC1_RGB_SRGB_BLOCK)
    {
        uint32_t pixelCount = width * height;
        uint8_t* pData = new uint8_t[pixelCount * 4];
        for (uint32_t i = 0; i < pixelCount; i++)
        {
            pData[i * 4 + 0] = imageData[i * 3 + 0];
            pData[i * 4 + 1] = imageData[i * 3 + 1];
            pData[i * 4 + 2] = imageData[i * 3 + 2];
            pData[i * 4 + 3] = 255;
        }
        
        CompressDXT1(pDest, pData, width, height, width * 4);
        
        delete [] pData;
    }
    else if (vkFormat == VK_FORMAT_BC7_UNORM_BLOCK || vkFormat == VK_FORMAT_BC7_SRGB_BLOCK)
    {
        CompressBC7(pDest, imageData, width, height, width * 4);
    }
    else if (vkFormat == VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG || vkFormat == VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)
    {
        uint32_t pixelCount = width * height;
        uint8_t* pData = new uint8_t[pixelCount * 4];
        for (uint32_t i = 0; i < pixelCount; i++)
        {
            pData[i * 4 + 0] = imageData[i * 3 + 0];
            pData[i * 4 + 1] = imageData[i * 3 + 1];
            pData[i * 4 + 2] = imageData[i * 3 + 2];
            pData[i * 4 + 3] = 255;
        }
        CompressPVRRGBA4Bpp(pDest, pData, width, height);
        
        delete [] pData;
    }
    // 非压缩格式
    else
    {
        memcpy(pDest, imageData, imageBytes);
    }
}

std::vector<uint8_t> TextureImporter::GenerateKTXData(imagecodec::VImagePtr image, const TextureImportSettings& textureImportSettings)
{
    bool generateMipmap = textureImportSettings.mipmapMode != MipmapMode::None;
    KTXFormat ktxFormat = CreateKTXFormat(image->GetFormat());

    uint32_t width = image->GetWidth();
    uint32_t height = image->GetHeight();

    uint32_t numMipLevels = 1;
    if (generateMipmap)
    {
        numMipLevels = imagecodec::ImageUtil::CalcNumMipLevels(width, height);
    }

    ktxTextureCreateInfo createInfoKTX = {};
    createInfoKTX.glInternalformat = ktxFormat.glInternalformat;
    createInfoKTX.vkFormat = ktxFormat.vkFormat;
    createInfoKTX.baseWidth = width;
    createInfoKTX.baseHeight = height;
    createInfoKTX.baseDepth = 1u;
    createInfoKTX.numDimensions = 2u;
    createInfoKTX.numLevels = numMipLevels;
    createInfoKTX.numLayers = 1u;
    createInfoKTX.numFaces = 1u;
    createInfoKTX.generateMipmaps = KTX_FALSE;
    ktxTexture1* textureKTX1 = nullptr;
    ktxTexture1_Create(&createInfoKTX, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX1);

    uint32_t w = width;
    uint32_t h = height;

    uint8_t* pTmpData = (uint8_t*)baselib::AlignedMalloc(width * height * image->GetBytesPerPixels() * 2, 64);
    uint8_t* pFormatData = (uint8_t*)baselib::AlignedMalloc(width * height * image->GetBytesPerPixels() * 2, 64);
    
    {
        size_t offset = 0;
        ktxTexture_GetImageOffset(ktxTexture(textureKTX1), 0, 0, 0, &offset);
        
        uint32_t bytesForImage = width * height * image->GetBytesPerPixels();
        CompressTextureInner(image->GetImageData(), w, h, ktxTexture_GetData(ktxTexture(textureKTX1)) + offset, ktxFormat.vkFormat, bytesForImage);

        h = h > 1 ? h >> 1 : 1;
        w = w > 1 ? w >> 1 : 1;
    }

    for (uint32_t i = 1; i != numMipLevels; ++i)
    {
        size_t offset = 0;
        ktxTexture_GetImageOffset(ktxTexture(textureKTX1), i, 0, 0, &offset);

        stbir_resize((const unsigned char*)image->GetImageData(), width, height, 0, pTmpData, w, h, 0,
                     ktxFormat.stbLayout, ktxFormat.stbDatatype, ktxFormat.stbEdge, ktxFormat.stbFilter);
        
        uint32_t bytesForImage = w * h * image->GetBytesPerPixels();

        CompressTextureInner(pTmpData, w, h, ktxTexture_GetData(ktxTexture(textureKTX1)) + offset, ktxFormat.vkFormat, bytesForImage);

        h = h > 1 ? h >> 1 : 1;
        w = w > 1 ? w >> 1 : 1;
    }

    baselib::AlignedFree(pTmpData);
    baselib::AlignedFree(pFormatData);
    
    ktx_uint8_t* ktxData = nullptr;
    ktx_size_t ktxDataSize = 0;

    ktxTexture_WriteToMemory(ktxTexture(textureKTX1), &ktxData, &ktxDataSize);
    
    std::vector<uint8_t> resultData;
    resultData.resize(ktxDataSize);
    memcpy(resultData.data(), ktxData, ktxDataSize);
    
    free(ktxData);
    ktxTexture_Destroy(ktxTexture(textureKTX1));

    return std::move(resultData);
}

bool TextureImporter::SaveTextureFile(const std::string& textureFilePath,
                                    const std::vector<uint8_t>& ktxData,
                                    const std::string& originalFileName)
{
	ByteVectorPtr encodedData = AssetManager::TextureMessageUtil::EncodeTextureMessage(ktxData.data(), ktxData.size());

	// 5. 创建资产文件头
	uint32_t flags = AssetManager::AssetFileFlags::NONE;
	if (mMeta.isCompressed)
	{
		flags |= AssetManager::AssetFileFlags::COMPRESSED;
	}

	// 计算 protobuf 数据的 hash
	uint64_t hash = AssetManager::AssetFileHeaderUtil::ComputeHash(encodedData->data(), encodedData->size());

	AssetManager::AssetFileHeader header = AssetManager::AssetFileHeaderUtil::CreateHeader(
		AssetManager::AssetType::Texture,
		originalFileName,
		hash,
		encodedData->size(),
		flags
	);

	// 6. 确保目标目录存在
	fs::path filePath(textureFilePath);
	fs::path parentDir = filePath.parent_path();
	if (!fs::exists(parentDir))
	{
		fs::create_directories(parentDir);
	}

	// 7. 写入文件：先写文件头，再写 protobuf 数据
	std::ofstream outFile(textureFilePath, std::ios::binary);
	if (!outFile.is_open())
	{
		LOG_ERROR("Failed to open texture file for writing: %s", textureFilePath.c_str());
		return false;
	}

	// 写入文件头
	if (!AssetManager::AssetFileHeaderUtil::WriteHeader(outFile, header))
	{
		LOG_ERROR("Failed to write asset file header");
		outFile.close();
		return false;
	}

	// 写入 protobuf 数据
	outFile.write(reinterpret_cast<const char*>(encodedData->data()), encodedData->size());
	outFile.close();

	LOG_INFO("Saved texture file: %s (size: %zu bytes)", textureFilePath.c_str(), encodedData->size());

	return true;
}

// ==================== 路径处理 ====================

std::string TextureImporter::GetMetaFilePath(const std::string& sourceFilePath) const
{
	return sourceFilePath + ".meta";
}

std::string TextureImporter::GetCacheDirectoryPath(const std::string& projectRootPath) const
{
	return (fs::path(projectRootPath) / ".gnx" / "Cache").string();
}

std::string TextureImporter::GetTextureFilePath(uint64_t hash, const std::string& projectRootPath) const
{
	fs::path cacheDir = GetCacheDirectoryPath(projectRootPath);
	return (cacheDir / (std::to_string(hash) + ".texture")).string();
}

std::string TextureImporter::GetThumbnailFilePath(uint64_t hash, const std::string& projectRootPath)
{
	fs::path cacheDir = (fs::path(projectRootPath) / ".gnx" / "Cache");
	return (cacheDir / (std::to_string(hash) + "_thumb.png")).string();
}

bool TextureImporter::GenerateThumbnail(imagecodec::VImagePtr image, uint64_t hash, const std::string& projectRootPath, uint32_t thumbnailSize)
{
	// 1. 计算缩略图尺寸（保持宽高比）
	uint32_t srcWidth = image->GetWidth();
	uint32_t srcHeight = image->GetHeight();

	// 如果原始图像已经很小，直接使用原始大小
	if (srcWidth <= thumbnailSize && srcHeight <= thumbnailSize)
	{
		thumbnailSize = std::max(srcWidth, srcHeight);
	}

	// 计算目标宽高（保持宽高比）
	uint32_t dstWidth, dstHeight;
	if (srcWidth > srcHeight)
	{
		dstWidth = thumbnailSize;
		dstHeight = static_cast<uint32_t>(static_cast<float>(srcHeight) * thumbnailSize / srcWidth);
	}
	else
	{
		dstHeight = thumbnailSize;
		dstWidth = static_cast<uint32_t>(static_cast<float>(srcWidth) * thumbnailSize / srcHeight);
	}

	// 2. 确定图像格式
	imagecodec::ImagePixelFormat srcFormat = image->GetFormat();
	stbir_pixel_layout stbLayout = STBIR_RGBA;
	stbir_datatype stbDatatype = STBIR_TYPE_UINT8;

	// 转换格式为 STBIR 可识别的格式
	switch (srcFormat)
	{
	case imagecodec::FORMAT_RGB8:
	case imagecodec::FORMAT_SRGB8:
		stbLayout = STBIR_RGB;
		break;
	case imagecodec::FORMAT_RGBA8:
	case imagecodec::FORMAT_SRGB8_ALPHA8:
		stbLayout = STBIR_RGBA;
		break;
	case imagecodec::FORMAT_GRAY8:
		stbLayout = STBIR_1CHANNEL;
		break;
	case imagecodec::FORMAT_GRAY8_ALPHA8:
		stbLayout = STBIR_2CHANNEL;
		break;
	case imagecodec::FORMAT_RGBA32Float:
		stbLayout = STBIR_RGBA;
		stbDatatype = STBIR_TYPE_FLOAT;
		break;
	case imagecodec::FORMAT_RGB32Float:
		stbLayout = STBIR_RGB;
		stbDatatype = STBIR_TYPE_FLOAT;
		break;
	default:
		LOG_WARN("Unsupported image format for thumbnail: %d", srcFormat);
		return false;
	}

	// 3. 分配缩略图内存
	size_t srcBytesPerPixel = image->GetBytesPerPixels();
	size_t bytesPerChannel = (stbDatatype == STBIR_TYPE_FLOAT) ? 4 : 1;
	size_t numChannels = (stbLayout == STBIR_RGBA) ? 4 : (stbLayout == STBIR_RGB) ? 3 : (stbLayout == STBIR_2CHANNEL) ? 2 : 1;
	size_t dstBytesPerPixel = bytesPerChannel * numChannels;
	size_t dstDataSize = dstWidth * dstHeight * dstBytesPerPixel;

	std::vector<uint8_t> dstData(dstDataSize);

	// 4. 使用 stbir_resize 进行缩放
	const uint8_t* srcData = image->GetImageData();
	void* result = stbir_resize(
		srcData, srcWidth, srcHeight, 0,
		dstData.data(), dstWidth, dstHeight, 0,
		stbLayout, stbDatatype,
		STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT
	);

	if (result == nullptr)
	{
		LOG_ERROR("Failed to resize image for thumbnail");
		return false;
	}

	imagecodec::VImage thumbnail;
	imagecodec::ImagePixelFormat dstFormat = image->GetFormat();

	thumbnail.SetImageInfo(dstFormat, dstWidth, dstHeight, result, nullptr);

	// 6. 保存为 PNG 文件
	std::string thumbnailPath = GetThumbnailFilePath(hash, projectRootPath);

	// 确保目录存在
	fs::path filePath(thumbnailPath);
	fs::path parentDir = filePath.parent_path();
	if (!fs::exists(parentDir))
	{
		fs::create_directories(parentDir);
	}

	// 使用 ImageEncoder 保存为 PNG
	bool success = imagecodec::ImageEncoder::EncodeFile(
		thumbnailPath.c_str(),
		thumbnail,
		imagecodec::kPNG_Format,
		90
	);

	if (!success)
	{
		LOG_ERROR("Failed to save thumbnail: %s", thumbnailPath.c_str());
		return false;
	}

	LOG_INFO("Generated thumbnail: %s (%ux%u)", thumbnailPath.c_str(), dstWidth, dstHeight);
	return true;
}

// ==================== 工厂方法 ====================

bool TextureImporter::ImportFromMemory(const uint8_t* data, size_t size, 
                                     const std::string& fileName, 
                                     const std::string& projectRootPath)
{
	// TODO: 实现从内存导入
	// 用于嵌入式纹理（如模型文件中的纹理）
	return false;
}

bool TextureImporter::ImportFromRawPixels(const uint8_t* data, uint32_t width, uint32_t height, 
                                        imagecodec::ImagePixelFormat format, 
                                        const std::string& fileName, 
                                        const std::string& projectRootPath)
{
	// TODO: 实现从原始像素导入
	// 用于程序化纹理
	return false;
}

NS_ASSETPROCESS_END
