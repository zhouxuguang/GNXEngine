#include "AssetManager.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <iostream>
#include <algorithm>
#include <filesystem>

NS_ASSETMANAGER_BEGIN

AssetManager* AssetManager::sInstance = nullptr;

AssetManager::AssetManager()
	: mInitialized(false)
{
}

AssetManager::~AssetManager()
{
	UnloadAllAssets();
}

AssetManager* AssetManager::GetInstance()
{
	return sInstance;
}

bool AssetManager::Initialize(const std::string& rootPath)
{
	if (sInstance)
	{
		return true; // 已初始化
	}

	sInstance = new AssetManager();
	sInstance->mRootPath = rootPath;

	// 确保根目录存在
	if (!baselib::FileUtil::IsDir(rootPath))
	{
		std::cerr << "AssetManager: Root directory does not exist: " << rootPath << std::endl;
		delete sInstance;
		sInstance = nullptr;
		return false;
	}

	std::cout << "AssetManager initialized with root: " << rootPath << std::endl;
	sInstance->mInitialized = true;
	return true;
}

void AssetManager::Shutdown()
{
	if (!sInstance)
	{
		return;
	}

	delete sInstance;
	sInstance = nullptr;
}

TextureAsset* AssetManager::LoadTexture(const std::string& path)
{
	return LoadTextureInternal(path, false);
}



void AssetManager::LoadTextureAsync(const std::string& path,
                                   std::function<void(TextureAsset*)> callback)
{
	// TODO: 实现异步加载
	// 这里需要线程池的支持
	TextureAsset* texture = LoadTextureInternal(path, true);
	if (callback)
	{
		callback(texture);
	}
}



TextureAsset* AssetManager::LoadTextureInternal(const std::string& path, bool async)
{
	if (!mInitialized)
	{
		std::cerr << "AssetManager not initialized!" << std::endl;
		return nullptr;
	}

	// 构建完整路径（支持带或不带.texture扩展名）
	std::string texturePath = path;
	if (path.find(".texture") == std::string::npos)
	{
		texturePath = path + ".texture";
	}
	std::string fullPath = mRootPath + "/" + texturePath;

	// 检查是否已加载（使用hash作为key）
	std::string hashKey = path;
	if (hashKey.find(".texture") != std::string::npos)
	{
		hashKey = hashKey.substr(0, hashKey.find(".texture"));
	}
	
	TextureAsset* existing = FindTexture(hashKey);
	if (existing)
	{
		existing->AddRef();
		return existing;
	}

	// 加载纹理（使用CreateFromTextureMessageFile替代CreateFromFiles）
    TextureAsset* texture = nullptr;
	if (!texture)
	{
		std::cerr << "Failed to load texture: " << path << std::endl;
		return nullptr;
	}

	// 添加到缓存
	AddToCache(texture->GetGUID(), texture);

	std::cout << "Loaded texture: " << path << std::endl;
	return texture;
}



Asset* AssetManager::FindAsset(const std::string& guid)
{
	auto it = mAssets.find(guid);
	if (it != mAssets.end())
	{
		return it->second;
	}
	return nullptr;
}

TextureAsset* AssetManager::FindTexture(const std::string& name)
{
	// 先直接查找
	auto it = mTextures.find(name);
	if (it != mTextures.end())
	{
		return it->second;
	}
	
	// 如果没找到，尝试去除扩展名后查找
	std::string nameNoExt = name;
	size_t extPos = nameNoExt.find(".texture");
	if (extPos != std::string::npos)
	{
		nameNoExt = nameNoExt.substr(0, extPos);
	}
	it = mTextures.find(nameNoExt);
	if (it != mTextures.end())
	{
		return it->second;
	}
	
	return nullptr;
}



std::vector<TextureAsset*> AssetManager::GetAllTextures() const
{
	std::vector<TextureAsset*> textures;
	textures.reserve(mTextures.size());

	for (const auto& pair : mTextures)
	{
		textures.push_back(pair.second);
	}

	return textures;
}



void AssetManager::UnloadAsset(Asset* asset)
{
	if (!asset)
	{
		return;
	}

	// 检查引用计数
	if (asset->GetRefCount() > 0)
	{
		std::cerr << "Cannot unload asset with non-zero reference count!" << std::endl;
		return;
	}

	// 从缓存中移除
	std::string guid = asset->GetGUID();
	mAssets.erase(guid);

	if (asset->GetType() == AssetType::Texture)
	{
		TextureAsset* texture = static_cast<TextureAsset*>(asset);
		mTextures.erase(texture->GetName());
	}

	// 从GPU释放并卸载
	asset->ReleaseFromGPU();
	asset->Unload();

	std::cout << "Unloaded asset: " << guid << std::endl;
}

void AssetManager::UnloadUnusedAssets()
{
	std::vector<std::string> toUnload;

	// 收集未使用的资源
	for (const auto& pair : mAssets)
	{
		if (pair.second->GetRefCount() == 0)
		{
			toUnload.push_back(pair.first);
		}
	}

	// 卸载未使用的资源
	for (const std::string& guid : toUnload)
	{
		Asset* asset = mAssets[guid];
		UnloadAsset(asset);
	}

	if (!toUnload.empty())
	{
		std::cout << "Unloaded " << toUnload.size() << " unused assets" << std::endl;
	}
}

void AssetManager::UnloadAllAssets()
{
	// 复制资源列表（避免在迭代时修改）
	std::vector<std::string> guids;
	guids.reserve(mAssets.size());

	for (const auto& pair : mAssets)
	{
		guids.push_back(pair.first);
	}

	// 卸载所有资源
	for (const std::string& guid : guids)
	{
		Asset* asset = mAssets[guid];
		UnloadAsset(asset);
	}

	mAssets.clear();
	mTextures.clear();

	std::cout << "Unloaded all assets" << std::endl;
}

bool AssetManager::ReloadAsset(const std::string& path)
{
	Asset* asset = FindAsset(path);
	if (!asset)
	{
		return false;
	}

	return asset->Reload();
}

int AssetManager::ReloadChangedAssets()
{
	int reloadedCount = 0;

	// TODO: 实现文件修改时间获取
	// 暂时禁用ReloadChangedAssets功能
	return reloadedCount;
}

uint64_t AssetManager::GetTotalMemoryUsage() const
{
	uint64_t totalMemory = 0;

	for (const auto& pair : mAssets)
	{
		totalMemory += pair.second->GetMemorySize();
	}

	return totalMemory;
}

uint64_t AssetManager::GetGPUMemoryUsage() const
{
	uint64_t gpuMemory = 0;

	for (const auto& pair : mAssets)
	{
		if (pair.second->IsOnGPU())
		{
			gpuMemory += pair.second->GetMemorySize();
		}
	}

	return gpuMemory;
}

uint32_t AssetManager::GetLoadedAssetCount() const
{
	return static_cast<uint32_t>(mAssets.size());
}

void AssetManager::GetAssetCountByType(std::unordered_map<AssetType, uint32_t>& counts) const
{
	counts.clear();

	for (const auto& pair : mAssets)
	{
		AssetType type = pair.second->GetType();
		counts[type]++;
	}
}

void AssetManager::PrintStatistics() const
{
	std::cout << "=== Asset Manager Statistics ===" << std::endl;
	std::cout << "Total Assets: " << mAssets.size() << std::endl;

	std::unordered_map<AssetType, uint32_t> counts;
	GetAssetCountByType(counts);

	for (const auto& pair : counts)
	{
		std::cout << GetAssetTypeName(pair.first) << ": " << pair.second << std::endl;
	}

	std::cout << "Memory Usage: " << (GetTotalMemoryUsage() / 1024.0 / 1024.0) << " MB" << std::endl;
	std::cout << "GPU Memory: " << (GetGPUMemoryUsage() / 1024.0 / 1024.0) << " MB" << std::endl;
	std::cout << "=============================" << std::endl;
}

template<typename T>
void AssetManager::AddToCache(const std::string& guid, T* asset)
{
	// 添加到主资源映射
	mAssets[guid] = asset;

	// 根据类型添加到特定映射
	if (asset->GetType() == AssetType::Texture)
	{
		TextureAsset* texture = static_cast<TextureAsset*>(asset);
		mTextures[texture->GetName()] = texture;
	}
}

TextureAsset* AssetManager::LoadTextureByHash(uint64_t hash)
{
	if (!mInitialized)
	{
		std::cerr << "AssetManager not initialized!" << std::endl;
		return nullptr;
	}

	// 构建.texture文件路径（在.gnx目录中）
	std::string textureFilePath = mRootPath + "/" + std::to_string(hash) + ".texture";
	std::string guid = std::to_string(hash);

	// 检查是否已加载
	TextureAsset* existing = FindTexture(guid);
	if (existing)
	{
		existing->AddRef();
		return existing;
	}

	// 加载纹理（使用CreateFromTextureMessageFile）
	TextureAsset* texture = nullptr;
	if (!texture)
	{
		std::cerr << "Failed to load texture: " << textureFilePath << std::endl;
		return nullptr;
	}

	// 添加到缓存
	AddToCache(texture->GetGUID(), texture);

	std::cout << "Loaded texture by hash: " << hash << std::endl;
	return texture;
}

NS_ASSETMANAGER_END
