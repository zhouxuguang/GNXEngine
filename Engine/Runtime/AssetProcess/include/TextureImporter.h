#ifndef GNX_ENGINE_TEXTURE_IMPORTER_INCLUDE
#define GNX_ENGINE_TEXTURE_IMPORTER_INCLUDE

#include "AssetProcessDefine.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include <string>
#include <functional>
#include <memory>

NS_ASSETPROCESS_BEGIN

// 纹理导入设置
struct TextureImportSettings
{
    //
};

/**
 * 纹理导入器
 * 负责将原始纹理文件（jpg/png等）导入为引擎可用的格式
 * 
 * 新的导入流程（类似 Unity）：
 * 1. 源文件拷贝到 Assets/ 目录（用户可编辑）
 * 2. 生成 .meta 文件记录导入设置和 hash
 * 3. 压缩后的纹理保存在 .gnx/Cache/{hash}.texture
 */
class ASSET_PROCESS_API TextureImporter
{
public:
	TextureImporter();
	~TextureImporter();

	// ==================== 主要导入接口 ====================

	/**
	 * 导入纹理（完整流程）
	 * @param sourceFilePath 源文件路径（可以是任意路径的图片文件）
	 * @param currentDir 当前目录（图片文件会被拷贝到这个目录）
	 * @return 导入是否成功
	 */
	bool Import(const std::string& sourceFilePath, const std::string& currentDir);

	/**
	 * 检查是否需要重新导入
	 * @param sourceFilePath 源文件路径
	 * @param projectRootPath 项目根目录
	 * @return true 如果需要重新导入
	 */
	bool NeedsReimport(const std::string& sourceFilePath, const std::string& projectRootPath);

	/**
	 * 删除导入的纹理
	 * @param sourceFilePath 源文件路径
	 * @param projectRootPath 项目根目录
	 */
	void RemoveImportedTexture(const std::string& sourceFilePath, const std::string& projectRootPath);

	// ==================== 获取信息 ====================

	/**
	 * 获取源文件 hash
	 */
	uint64_t GetSourceFileHash() const;

	/**
	 * 获取纹理 hash（生成的 .texture 文件）
	 */
	uint64_t GetTextureHash() const;

	/**
	 * 获取导入设置
	 */
	const TextureImportSettings& GetImportSettings() const;

	/**
	 * 获取导入设置（可修改）
	 */
	TextureImportSettings& GetImportSettings();

	// ==================== 工厂方法 ====================

	/**
	 * 从内存数据导入纹理（用于嵌入式纹理）
	 * @param data 纹理数据
	 * @param size 数据大小
	 * @param fileName 文件名（用于生成 meta 信息）
	 * @param projectRootPath 项目根目录
	 * @return 导入是否成功
	 */
	static bool ImportFromMemory(const uint8_t* data, size_t size, 
	                             const std::string& fileName, 
	                             const std::string& projectRootPath);

	/**
	 * 从原始像素数据导入纹理（用于程序化纹理）
	 * @param data 像素数据
	 * @param width 宽度
	 * @param height 高度
	 * @param format 像素格式
	 * @param fileName 文件名
	 * @param projectRootPath 项目根目录
	 * @return 导入是否成功
	 */
	static bool ImportFromRawPixels(const uint8_t* data, uint32_t width, uint32_t height, 
	                                  imagecodec::ImagePixelFormat format, 
	                                  const std::string& fileName, 
	                                  const std::string& projectRootPath);

private:
	// ==================== 导入流程内部方法 ====================

	/**
	 * 计算源文件 hash
	 */
	uint64_t CalculateSourceFileHash(const std::string& filePath);

	/**
	 * 加载现有的 .meta 文件
	 */
	bool LoadMetaFile(const std::string& metaFilePath);

	/**
	 * 保存 .meta 文件
	 */
	bool SaveMetaFile(const std::string& metaFilePath);

	/**
	 * 应用默认导入设置
	 */
	void ApplyDefaultSettings(const std::string& fileName, imagecodec::ImagePixelFormat format);

	/**
	 * 根据导入设置压缩纹理
	 */
	bool CompressTexture(imagecodec::VImagePtr image, const std::string& textureFilePath);

	/**
	 * 生成 KTX 数据
	 */
	std::vector<uint8_t> GenerateKTXData(imagecodec::VImagePtr image);

	/**
	 * 保存 .texture 文件到 .gnx/Cache
	 */
	bool SaveTextureFile(const std::string& textureFilePath, 
	                      const std::vector<uint8_t>& ktxData, 
	                      const std::string& originalFileName);

	// ==================== 路径处理 ====================

	/**
	 * 获取 .meta 文件路径
	 */
	std::string GetMetaFilePath(const std::string& sourceFilePath) const;

	/**
	 * 获取 .gnx/Cache 目录路径
	 */
	std::string GetCacheDirectoryPath(const std::string& projectRootPath) const;

	/**
	 * 获取 .texture 文件路径
	 */
	std::string GetTextureFilePath(uint64_t hash, const std::string& projectRootPath) const;

	// ==================== 成员变量 ====================

	TextureImportSettings mSettings;
	
	uint64_t mSourceFileHash;
	uint64_t mTextureHash;
	
	std::string mSourceFilePath;
	std::string mCurrentDir;
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_TEXTURE_IMPORTER_INCLUDE
