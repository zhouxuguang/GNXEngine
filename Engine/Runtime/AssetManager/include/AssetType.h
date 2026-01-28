#ifndef GNX_ENGINE_ASSET_TYPE_INCLUDE
#define GNX_ENGINE_ASSET_TYPE_INCLUDE

#include <cstdint>
#include "AssetDefine.h"

NS_ASSETMANAGER_BEGIN

/**
 * 资源类型枚举
 * 用于区分和管理不同类型的资源
 */
enum class AssetType : uint8_t
{
	Unknown = 0,      // 未知类型
	Texture = 1,       // 纹理资源
	Material = 2,       // 材质资源
	Shader = 3,         // 着色器资源
	Audio = 4,          // 音频资源
	Animation = 5,      // 动画资源
	Scene = 6,          // 场景资源
	Font = 7,           // 字体资源
	Cubemap = 8,        // 立方体贴图
	Lightmap = 9,      // 光照贴图
	ReflectionProbe = 10, // 反射探针
	Mesh = 11           // 网格资源
};

/**
 * 资源状态枚举
 * 描述资源的生命周期状态
 */
enum class AssetState : uint8_t
{
	Unloaded = 0,     // 未加载
	Loading = 1,       // 正在加载
	Loaded = 2,        // 已加载到内存
	Uploading = 3,      // 正在上传到GPU
	Ready = 4,         // 准备就绪（已上传到GPU）
	Error = 5           // 加载错误
};

/**
 * 获取资源类型名称
 */
inline const char* GetAssetTypeName(AssetType type)
{
	switch (type)
	{
	case AssetType::Unknown: return "Unknown";
	case AssetType::Texture: return "Texture";
	case AssetType::Material: return "Material";
	case AssetType::Shader: return "Shader";
	case AssetType::Audio: return "Audio";
	case AssetType::Animation: return "Animation";
	case AssetType::Scene: return "Scene";
	case AssetType::Font: return "Font";
	case AssetType::Cubemap: return "Cubemap";
	case AssetType::Lightmap: return "Lightmap";
	case AssetType::ReflectionProbe: return "ReflectionProbe";
	case AssetType::Mesh: return "Mesh";
	default: return "Unknown";
	}
}

NS_ASSETMANAGER_END

#endif // !GNX_ENGINE_ASSET_TYPE_INCLUDE
