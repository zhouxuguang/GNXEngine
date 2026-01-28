#include "AssetManager.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <iostream>
#include <algorithm>
#include <filesystem>

NS_ASSETMANAGER_BEGIN

AssetManager* AssetManager::s_instance = nullptr;

AssetManager::AssetManager()
	: m_initialized(false)
{
}

AssetManager::~AssetManager()
{
	UnloadAllAssets();
}

AssetManager* AssetManager::GetInstance()
{
	return s_instance;
}

bool AssetManager::Initialize(const std::string& rootPath)
{
	if (s_instance)
	{
		return true; // 已初始化
	}

	s_instance = new AssetManager();
	s_instance->m_rootPath = rootPath;

	// 确保根目录存在
	if (!baselib::FileUtil::IsDir(rootPath))
	{
		std::cerr << "AssetManager: Root directory does not exist: " << rootPath << std::endl;
		delete s_instance;
		s_instance = nullptr;
		return false;
	}

	std::cout << "AssetManager initialized with root: " << rootPath << std::endl;
	s_instance->m_initialized = true;
	return true;
}

void AssetManager::Shutdown()
{
	if (!s_instance)
	{
		return;
	}

	delete s_instance;
	s_instance = nullptr;
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
	if (!m_initialized)
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
	std::string fullPath = m_rootPath + "/" + texturePath;

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
	TextureAsset* texture = TextureAsset::CreateFromTextureMessageFile(fullPath);
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
	auto it = m_assets.find(guid);
	if (it != m_assets.end())
	{
		return it->second;
	}
	return nullptr;
}

TextureAsset* AssetManager::FindTexture(const std::string& name)
{
	// 先直接查找
	auto it = m_textures.find(name);
	if (it != m_textures.end())
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
	it = m_textures.find(nameNoExt);
	if (it != m_textures.end())
	{
		return it->second;
	}
	
	return nullptr;
}



std::vector<TextureAsset*> AssetManager::GetAllTextures() const
{
	std::vector<TextureAsset*> textures;
	textures.reserve(m_textures.size());

	for (const auto& pair : m_textures)
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
	m_assets.erase(guid);

	if (asset->GetType() == AssetType::Texture)
	{
		TextureAsset* texture = static_cast<TextureAsset*>(asset);
		m_textures.erase(texture->GetName());
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
	for (const auto& pair : m_assets)
	{
		if (pair.second->GetRefCount() == 0)
		{
			toUnload.push_back(pair.first);
		}
	}

	// 卸载未使用的资源
	for (const std::string& guid : toUnload)
	{
		Asset* asset = m_assets[guid];
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
	guids.reserve(m_assets.size());

	for (const auto& pair : m_assets)
	{
		guids.push_back(pair.first);
	}

	// 卸载所有资源
	for (const std::string& guid : guids)
	{
		Asset* asset = m_assets[guid];
		UnloadAsset(asset);
	}

	m_assets.clear();
	m_textures.clear();

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





std::vector<TextureAsset*> AssetManager::GetAllNormalMaps() const
{
	std::vector<TextureAsset*> normalMaps;

	for (const auto& pair : m_textures)
	{
		if (pair.second->IsNormalMap())
		{
			normalMaps.push_back(pair.second);
		}
	}

	return normalMaps;
}

std::vector<TextureAsset*> AssetManager::GetAllAlbedoMaps() const
{
	std::vector<TextureAsset*> albedoMaps;

	for (const auto& pair : m_textures)
	{
		if (pair.second->IsAlbedoMap())
		{
			albedoMaps.push_back(pair.second);
		}
	}

	return albedoMaps;
}

std::vector<TextureAsset*> AssetManager::GetAllRoughnessMaps() const
{
	std::vector<TextureAsset*> roughnessMaps;

	for (const auto& pair : m_textures)
	{
		if (pair.second->IsRoughnessMap())
		{
			roughnessMaps.push_back(pair.second);
		}
	}

	return roughnessMaps;
}

std::vector<TextureAsset*> AssetManager::GetAllMetallicMaps() const
{
	std::vector<TextureAsset*> metallicMaps;

	for (const auto& pair : m_textures)
	{
		if (pair.second->IsMetallicMap())
		{
			metallicMaps.push_back(pair.second);
		}
	}

	return metallicMaps;
}

uint64_t AssetManager::GetTotalMemoryUsage() const
{
	uint64_t totalMemory = 0;

	for (const auto& pair : m_assets)
	{
		totalMemory += pair.second->GetMemorySize();
	}

	return totalMemory;
}

uint64_t AssetManager::GetGPUMemoryUsage() const
{
	uint64_t gpuMemory = 0;

	for (const auto& pair : m_assets)
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
	return static_cast<uint32_t>(m_assets.size());
}

void AssetManager::GetAssetCountByType(std::unordered_map<AssetType, uint32_t>& counts) const
{
	counts.clear();

	for (const auto& pair : m_assets)
	{
		AssetType type = pair.second->GetType();
		counts[type]++;
	}
}

void AssetManager::PrintStatistics() const
{
	std::cout << "=== Asset Manager Statistics ===" << std::endl;
	std::cout << "Total Assets: " << m_assets.size() << std::endl;

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
	m_assets[guid] = asset;

	// 根据类型添加到特定映射
	if (asset->GetType() == AssetType::Texture)
	{
		TextureAsset* texture = static_cast<TextureAsset*>(asset);
		m_textures[texture->GetName()] = texture;
	}
}

TextureAsset* AssetManager::LoadTextureByHash(uint64_t hash)
{
	if (!m_initialized)
	{
		std::cerr << "AssetManager not initialized!" << std::endl;
		return nullptr;
	}

	// 构建.texture文件路径（在.gnx目录中）
	std::string textureFilePath = m_rootPath + "/" + std::to_string(hash) + ".texture";
	std::string guid = std::to_string(hash);

	// 检查是否已加载
	TextureAsset* existing = FindTexture(guid);
	if (existing)
	{
		existing->AddRef();
		return existing;
	}

	// 加载纹理（使用CreateFromTextureMessageFile）
	TextureAsset* texture = TextureAsset::CreateFromTextureMessageFile(textureFilePath);
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
