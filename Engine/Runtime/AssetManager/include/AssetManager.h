#ifndef GNX_ENGINE_ASSET_MANAGER_INCLUDE
#define GNX_ENGINE_ASSET_MANAGER_INCLUDE

#include "Asset.h"
#include "TextureAsset.h"
#include "ShaderAsset.h"
#include "AssetType.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

NS_ASSETMANAGER_BEGIN

/**
 * 资源管理器类
 * 负责所有资源（纹理、材质等）的加载、卸载和管理
 * 提供统一的资源访问接口，支持引用计数和内存管理
 */
class ASSET_MANAGER_API AssetManager
{
public:
	// ==================== 单例模式 ====================

	/**
	 * 获取资源管理器单例
	 */
	static AssetManager* GetInstance();

	/**
	 * 初始化资源管理器
	 * @param rootPath 资根目录（.gnx目录）
	 * @return 成功返回true
	 */
	static bool Initialize(const std::string& rootPath);

	/**
	 * 销毁资源管理器
	 */
	static void Shutdown();

	// ==================== 包内资源读取（跨平台） ====================

	/**
	 * 读取包内资源到内存（跨平台：Android assets / iOS bundle / PC 文件系统）
	 * @param relPath 包内相对路径（如 "Shader/BasePass.spirv.gnxasset"）
	 * @param outData 输出的字节数据
	 * @return 成功返回 true
	 */
	static bool LoadResource(const std::string& relPath, std::vector<uint8_t>& outData);

	/**
	 * 判断包内资源是否存在
	 * @param relPath 包内相对路径
	 * @return 存在返回 true
	 */
	static bool ResourceExists(const std::string& relPath);

	// ==================== 资源加载 ====================

	/**
	 * 加载纹理资源
	 * @param path 纹理资源路径（相对于.gnx目录）
	 * @return 加载的纹理资源，失败返回nullptr
	 */
	TextureAsset* LoadTexture(const std::string& path);

	/**
	 * 异步加载纹理资源
	 * @param path 纹理资源路径
	 * @param callback 加载完成回调
	 */
	void LoadTextureAsync(const std::string& path,
	                  std::function<void(TextureAsset*)> callback);

	/**
	 * 通过hash值加载纹理资源
	 * @param hash 资产的hash值
	 * @return 加载的纹理资源，失败返回nullptr
	 */
	TextureAsset* LoadTextureByHash(uint64_t hash);

	// ==================== Shader 资源 ====================

	/**
	 * 加载预编译 shader 资源（.gnxasset）
	 * @param filePath .gnxasset 文件完整路径
	 * @return 加载的 shader 资源，失败返回nullptr
	 * 已缓存则直接返回并 AddRef；未缓存则加载并加入缓存
	 */
	ShaderAsset* LoadShader(const std::string& filePath);

	/**
	 * 通过文件名（GUID）查找 shader 资源
	 * @param name 文件名（含扩展名，如 "GBufferPBR.vs.spirv.gnxasset"）
	 * @return 找到的 shader，不存在返回nullptr
	 */
	ShaderAsset* FindShader(const std::string& name);

	// ==================== 资源查找 ====================

	/**
	 * 通过GUID查找资源
	 * @param guid 资源GUID
	 * @return 找到的资源，不存在返回nullptr
	 */
	Asset* FindAsset(const std::string& guid);

	/**
	 * 通过名称查找纹理
	 * @param name 纹理名称
	 * @return 找到的纹理，不存在返回nullptr
	 */
	TextureAsset* FindTexture(const std::string& name);

	/**
	 * 获取所有纹理资源
	 */
	std::vector<TextureAsset*> GetAllTextures() const;

	// ==================== 资源卸载 ====================

	/**
	 * 卸载指定资源
	 * @param asset 要卸载的资源
	 */
	void UnloadAsset(Asset* asset);

	/**
	 * 卸载所有未使用的资源（引用计数为0）
	 */
	void UnloadUnusedAssets();

	/**
	 * 卸载所有资源
	 */
	void UnloadAllAssets();

	// ==================== 资源热重载 ====================

	/**
	 * 重新加载指定资源
	 * @param path 资源路径
	 * @return 成功返回true
	 */
	bool ReloadAsset(const std::string& path);

	/**
	 * 检查并重新加载已修改的资源
	 * @return 重新加载的资源数量
	 */
	int ReloadChangedAssets();

	// ==================== 内存管理 ====================

	/**
	 * 获取总内存使用量（字节）
	 */
	uint64_t GetTotalMemoryUsage() const;

	/**
	 * 获取GPU内存使用量（字节）
	 */
	uint64_t GetGPUMemoryUsage() const;

	/**
	 * 获取已加载的资源数量
	 */
	uint32_t GetLoadedAssetCount() const;

	/**
	 * 获取资源数量统计
	 */
	void GetAssetCountByType(std::unordered_map<AssetType, uint32_t>& counts) const;

	// ==================== 调试信息 ====================

	/**
	 * 打印资源使用统计
	 */
	void PrintStatistics() const;

private:
	AssetManager();
	~AssetManager();

	/**
	 * 内部加载纹理实现
	 */
	TextureAsset* LoadTextureInternal(const std::string& path, bool async);

	/**
	 * 将资源添加到缓存（按运行时类型分发到 mTextures / mShaders）
	 */
	void AddToCache(const std::string& guid, Asset* asset);

	std::string mRootPath;                              // 资根目录

	std::unordered_map<std::string, Asset*> mAssets;        // GUID到资源的映射
	std::unordered_map<std::string, TextureAsset*> mTextures;  // 名称到纹理的映射
	std::unordered_map<std::string, ShaderAsset*> mShaders;   // 名称到shader的映射

	static AssetManager* sInstance;
	bool mInitialized;
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_ASSET_MANAGER_INCLUDE
