#ifndef GNX_ENGINE_ASSET_REFERENCE_INCLUDE
#define GNX_ENGINE_ASSET_REFERENCE_INCLUDE

#include "AssetProcessDefine.h"
#include "Runtime/AssetManager/include/AssetDefine.h"
#include "Runtime/AssetManager/include/AssetReference.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include <string>

NS_ASSETPROCESS_BEGIN

/**
 * 资产引用类
 * 负责管理编辑器目录中的资产引用文件（.gnx文件）
 * 记录原始文件与.gnx目录中资产文件的映射关系
 */
class ASSET_PROCESS_API AssetReference
{
public:
	AssetReference();
	~AssetReference();

	// 设置资产哈希值
	void SetHash(uint64_t hash);

	// 设置原始文件名
	void SetOriginalFileName(const std::string& fileName);

	// 设置原始文件路径
	void SetOriginalPath(const std::string& path);

	// 设置资产类型
	void SetAssetType(AssetType assetType);

	// 获取资产哈希值
	uint64_t GetHash() const;

	// 获取原始文件名
	std::string GetOriginalFileName() const;

	// 获取原始文件路径
	std::string GetOriginalPath() const;

	// 获取资产类型
	AssetType GetAssetType() const;

	// 序列化为protobuf并保存到文件
	bool SaveToFile(const std::string& filePath);

	// 从文件加载引用数据
	bool LoadFromFile(const std::string& filePath);

	// 获取protobuf消息结构
	const AssetReferenceMessage& GetMessage() const;

private:
	AssetReferenceMessage m_message;

	// 字符串字段的存储
	std::string m_originalFileName;
	std::string m_originalPath;
	std::string m_importTime;
	std::string m_engineVersion;

	// 辅助函数：设置字符串回调
	void SetupStringCallbacks();
};

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_ASSET_REFERENCE_INCLUDE
