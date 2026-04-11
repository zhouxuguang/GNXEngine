#ifndef GNX_ENGINE_ASSET_INCLUDE
#define GNX_ENGINE_ASSET_INCLUDE

#include "AssetType.h"
#include "AssetDefine.h"
#include <string>
#include <cstdint>
#include <atomic>

NS_ASSETMANAGER_BEGIN

/**
 * 资源基类
 * 所有资源（纹理、网格、材质等）的抽象基类
 * 提供统一的资源管理接口，支持引用计数和生命周期管理
 */
class Asset
{
public:
	Asset();
	virtual ~Asset();

	/**
	 * 资源类型
	 */
	virtual AssetType GetType() const = 0;

	/**
	 * 资源唯一标识符（GUID）
	 */
	virtual const std::string& GetGUID() const = 0;

	/**
	 * 资源名称
	 */
	virtual const std::string& GetName() const = 0;

	/**
	 * 资源文件路径
	 */
	virtual const std::string& GetFilePath() const = 0;

	// ==================== 生命周期管理 ====================

	/**
	 * 加载资源（从磁盘加载到内存）
	 * @return 成功返回 true
	 */
	virtual bool Load() = 0;

	/**
	 * 卸载资源（从内存中卸载）
	 */
	virtual void Unload() = 0;

	/**
	 * 重新加载资源
	 * @return 成功返回 true
	 */
	virtual bool Reload() = 0;

	/**
	 * 检查资源是否已加载
	 */
	virtual bool IsLoaded() const = 0;

	/**
	 * 将资源上传到GPU
	 * @return 成功返回 true
	 */
	virtual bool UploadToGPU() = 0;

	/**
	 * 从GPU释放资源
	 */
	virtual void ReleaseFromGPU() = 0;

	/**
	 * 检查资源是否已在GPU上
	 */
	virtual bool IsOnGPU() const = 0;

	// ==================== 引用计数 ====================

	/**
	 * 增加引用计数
	 */
	void AddRef();

	/**
	 * 减少引用计数
	 * 如果引用计数降为0，资源可以被卸载
	 */
	void Release();

	/**
	 * 获取当前引用计数
	 */
	int GetRefCount() const;

	// ==================== 状态管理 ====================

	/**
	 * 获取当前资源状态
	 */
	AssetState GetState() const;

	/**
	 * 设置资源状态
	 */
	void SetState(AssetState state);

	// ==================== 元数据 ====================

	/**
	 * 获取原始文件大小（字节）
	 */
	uint64_t GetFileSize() const;

	/**
	 * 设置原始文件大小
	 */
	void SetFileSize(uint64_t size);

	/**
	 * 获取内存占用大小（字节）
	 */
	uint64_t GetMemorySize() const;

	/**
	 * 设置内存占用大小
	 */
	void SetMemorySize(uint64_t size);

	/**
	 * 获取最后修改时间
	 */
	int64_t GetLastModified() const;

	/**
	 * 设置最后修改时间
	 */
	void SetLastModified(int64_t timestamp);

protected:
	std::string mGuid;
	std::string mName;
	std::string mFilePath;
	std::atomic<int> mRefCount;
	AssetState mState;

	uint64_t mFileSize;
	uint64_t mMemorySize;
	int64_t mLastModified;
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_ASSET_INCLUDE
