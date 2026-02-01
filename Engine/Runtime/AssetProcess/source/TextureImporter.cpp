#include "TextureImporter.h"
#include "AssetReference.h"
#include "ktx.h"
#include "Runtime/ImageCodec/include/ImageUtil.h"
#include "TextureProcess/stb_image_resize2.h"
#include "DXTCompressor.h"
#include "PVRCompressor.h"
#include "ASTCCompressor.h"
#include "Runtime/BaseLib/include/AlignedMalloc.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

NS_ASSETPROCESS_BEGIN

// 导入器版本（用于检测设置格式变化）
const uint32_t IMPORTER_VERSION = 1;

TextureImporter::TextureImporter() : mSourceFileHash(0), mTextureHash(0)
{
}

TextureImporter::~TextureImporter()
{
}

// ==================== 主要导入接口 ====================

bool TextureImporter::Import(const std::string& sourceFilePath, const std::string& currentDir)
{
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

	// 11. 应用默认设置（如果是首次导入）
	if (!metaExists)
	{
		std::string fileNameStr = fs::path(targetFilePath).filename().string();
		ApplyDefaultSettings(fileNameStr, image->GetFormat());
	}

	// 12. 压缩纹理
    std::string textureFilePath = GetTextureFilePath(mSourceFileHash, currentDir);
	if (!CompressTexture(image, textureFilePath))
	{
		return false;
	}

	// 13. 保存 .meta 文件
	if (!SaveMetaFile(metaFilePath))
	{
		return false;
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
	TextureImporter tempImporter;
	if (!tempImporter.LoadMetaFile(metaFilePath))
	{
		return true;  // meta 文件损坏
	}

	// 检查源文件 hash 是否变化
	uint64_t currentHash = tempImporter.CalculateSourceFileHash(sourceFilePath);
	if (currentHash != tempImporter.GetSourceFileHash())
	{
		return true;
	}

	return false;
}

void TextureImporter::RemoveImportedTexture(const std::string& sourceFilePath, const std::string& projectRootPath)
{
	// 加载 meta 文件获取 texture hash
	std::string metaFilePath = GetMetaFilePath(sourceFilePath);
	TextureImporter tempImporter;
	
	if (tempImporter.LoadMetaFile(metaFilePath))
	{
		uint64_t textureHash = tempImporter.GetTextureHash();
		
		// 删除 .texture 文件
		std::string textureFilePath = GetTextureFilePath(textureHash, projectRootPath);
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

const TextureImportSettings& TextureImporter::GetImportSettings() const
{
    return mSettings;
}

TextureImportSettings& TextureImporter::GetImportSettings()
{
    return mSettings;
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
	// TODO: 实现 protobuf 反序列化
	// 这里需要使用 nanopb 库来反序列化 TextureImportSettings
	return false;
}

bool TextureImporter::SaveMetaFile(const std::string& metaFilePath)
{
	// 设置字符串回调
	// TODO: 实现 protobuf 序列化
	// 这里需要使用 nanopb 库来序列化 TextureImportSettings
	return false;
}

void TextureImporter::ApplyDefaultSettings(const std::string& fileName, imagecodec::ImagePixelFormat format)
{
	// 自动检测纹理类型
	std::string lowerFileName = fileName;
	std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

	
}

bool TextureImporter::CompressTexture(imagecodec::VImagePtr image, const std::string& textureFilePath)
{
	// 生成 KTX 数据
	std::vector<uint8_t> ktxData = GenerateKTXData(image);
	if (ktxData.empty())
	{
		return false;
	}

	// 计算纹理 hash（基于 KTX 数据）
//	baselib::NXGUID guid = CreateGUIDFromBinaryData(ktxData.data(), ktxData.size());
//	mTextureHash = guid.Data1;
//	mSettings.textureHash = mTextureHash;

	// 保存 .texture 文件
	return SaveTextureFile(textureFilePath, ktxData, fs::path(mSourceFilePath).filename().string());
}

std::vector<uint8_t> TextureImporter::GenerateKTXData(imagecodec::VImagePtr image)
{
	// TODO: 实现实际的 KTX 数据生成
	// 这里需要根据导入设置（压缩格式、Mipmap 等）生成 KTX 数据
	// 可以参考现有的 CreateKTXFormatData 函数
	
	// 暂时返回空
	return std::vector<uint8_t>();
}

bool TextureImporter::SaveTextureFile(const std::string& textureFilePath, 
                                    const std::vector<uint8_t>& ktxData, 
                                    const std::string& originalFileName)
{
	// 生成 TextureMessage
	//TextureMessage textureMsg = TextureMessage_init_default;
	
	// 设置基本属性
	//	textureMsg.textureType = mSettings.textureType;
	//	textureMsg.compressType = mSettings.enableCompression ? CompressType_ZLIB : CompressType_NONE;
	//	textureMsg.isSRGB = mSettings.sRGB;
	//	textureMsg.hasAlpha = mSettings.hasAlpha;
	//	textureMsg.isCompressed = mSettings.enableCompression;
	//	textureMsg.hash = mTextureHash;

	// 设置源信息
	// TODO: 设置 sourceFile, importTime, engineVersion
	
	// 设置 KTX 数据
	// TODO: 设置 imageData

	// TODO: 使用 AssetFileHeader 和 nanopb 序列化并保存
	
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
