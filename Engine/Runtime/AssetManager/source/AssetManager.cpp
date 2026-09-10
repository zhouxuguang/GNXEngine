#include "AssetManager.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include "Runtime/BaseLib/include/PreCompile.h"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

// 移动端包内资源读取：SDL_RWFromFile（Android 自动 AAssetManager / iOS 自动 bundle）。
// SDL 头仅在 .cpp 内部 include，不外泄到任何头文件。
#if GNX_OS_IOS || GNX_OS_ANDROID
#include "SDL_rwops.h"
#endif

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

#if GNX_OS_IOS || GNX_OS_ANDROID
	// 移动端：rootPath 是 GetProjectAssetDir() 基于 __FILE__ 推导的构建机路径，
	// FileUtil::IsDir 用 _stat/stat 检查真实文件系统必然失败（设备上不存在）。
	// 包内资源通过 LoadResource（SDL_RWFromFile）读取，不依赖 mRootPath，
	// 因此移动端跳过 IsDir 校验（漏洞13）。
	(void)rootPath;
#else
	// 桌面端：确保根目录存在
	if (!baselib::FileUtil::IsDir(rootPath))
	{
		std::cerr << "AssetManager: Root directory does not exist: " << rootPath << std::endl;
		delete sInstance;
		sInstance = nullptr;
		return false;
	}
#endif

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

bool AssetManager::SetProjectRoot(const std::string& rootPath)
{
	if (!sInstance || rootPath.empty() || !baselib::FileUtil::IsDir(rootPath))
		return false;
	sInstance->mProjectRootPath = rootPath;
	return true;
}

void AssetManager::ClearProjectRoot()
{
	if (!sInstance)
		return;
	const std::filesystem::path oldRoot =
		std::filesystem::path(sInstance->mProjectRootPath).lexically_normal();
	for (auto iterator = sInstance->mTexturesByPath.begin();
		 iterator != sInstance->mTexturesByPath.end();)
	{
		const std::filesystem::path assetPath(iterator->first);
		const auto relative = assetPath.lexically_relative(oldRoot);
		const bool inProject = !oldRoot.empty() && !relative.empty() &&
			*relative.begin() != "..";
		if (!inProject)
		{
			++iterator;
			continue;
		}
		Asset* asset = iterator->second;
		for (auto entry = sInstance->mAssets.begin(); entry != sInstance->mAssets.end();)
			entry = entry->second == asset ? sInstance->mAssets.erase(entry) : std::next(entry);
		auto byName = sInstance->mTextures.find(asset->GetName());
		if (byName != sInstance->mTextures.end() && byName->second == asset)
			sInstance->mTextures.erase(byName);
		iterator = sInstance->mTexturesByPath.erase(iterator);
		if (asset->GetRefCount() == 0)
		{
			asset->ReleaseFromGPU();
			asset->Unload();
			delete asset;
		}
		else
			sInstance->mRetiredAssets.push_back(asset);
	}
	for (const auto& entry : sInstance->mTexturesByPath)
		sInstance->mTextures.try_emplace(entry.second->GetName(), entry.second);
	for (auto iterator = sInstance->mShadersByPath.begin();
		 iterator != sInstance->mShadersByPath.end();)
	{
		const std::filesystem::path assetPath(iterator->first);
		const auto relative = assetPath.lexically_relative(oldRoot);
		const bool inProject = !oldRoot.empty() && !relative.empty() &&
			*relative.begin() != "..";
		if (!inProject)
		{
			++iterator;
			continue;
		}
		Asset* asset = iterator->second;
		for (auto entry = sInstance->mAssets.begin();
			 entry != sInstance->mAssets.end();)
			entry = entry->second == asset ? sInstance->mAssets.erase(entry)
			                               : std::next(entry);
		auto byName = sInstance->mShaders.find(asset->GetName());
		if (byName != sInstance->mShaders.end() && byName->second == asset)
			sInstance->mShaders.erase(byName);
		iterator = sInstance->mShadersByPath.erase(iterator);
		if (asset->GetRefCount() == 0)
		{
			asset->ReleaseFromGPU();
			asset->Unload();
			delete asset;
		}
		else
			sInstance->mRetiredAssets.push_back(asset);
	}
	for (const auto& entry : sInstance->mShadersByPath)
		sInstance->mShaders.try_emplace(entry.second->GetName(), entry.second);
	sInstance->mResourcePaths.clear();
	sInstance->mProjectRootPath.clear();
}

bool AssetManager::RegisterResourcePath(const std::string& guid,
	const std::string& path)
{
	if (!sInstance || guid.empty() || path.empty())
		return false;
	const std::filesystem::path resolved =
		std::filesystem::absolute(path).lexically_normal();
	if (!std::filesystem::is_regular_file(resolved))
		return false;
	sInstance->mResourcePaths[guid] = resolved.string();
	return true;
}

std::string AssetManager::ResolveResourcePath(const std::string& path)
{
#if GNX_OS_IOS || GNX_OS_ANDROID
	return path;
#else
	if (path.empty())
		return {};
	const std::filesystem::path requested(path);
	if (sInstance)
	{
		auto mapped = sInstance->mResourcePaths.find(path);
		if (mapped == sInstance->mResourcePaths.end() &&
			requested.extension() == ".texture")
			mapped = sInstance->mResourcePaths.find(requested.stem().string());
		if (mapped != sInstance->mResourcePaths.end())
			return mapped->second;
	}
	if (requested.is_absolute() && std::filesystem::is_regular_file(requested))
		return requested.lexically_normal().string();
	if (sInstance && !sInstance->mProjectRootPath.empty())
	{
		const auto candidate = std::filesystem::path(sInstance->mProjectRootPath) / requested;
		if (std::filesystem::is_regular_file(candidate))
			return candidate.lexically_normal().string();
	}
	if (sInstance)
	{
		const auto candidate = std::filesystem::path(sInstance->mRootPath) / requested;
		if (std::filesystem::is_regular_file(candidate))
			return candidate.lexically_normal().string();
	}
	return path;
#endif
}

bool AssetManager::LoadResource(const std::string& relPath, std::vector<uint8_t>& outData)
{
	outData.clear();

#if GNX_OS_IOS || GNX_OS_ANDROID
	// 移动端：SDL_RWFromFile 相对路径自动走包内（Android AAssetManager / iOS NSBundle）。
	// 用 SDL_LoadFile_RW（内部处理 size 未知/读满/分配），读完整文件到内存（漏洞7）。
	SDL_RWops* rw = SDL_RWFromFile(relPath.c_str(), "rb");
	if (!rw)
	{
		return false;
	}

	size_t dataSize = 0;
	void* data = SDL_LoadFile_RW(rw, &dataSize, 1);   // freesrc=1，内部 close 并释放
	if (!data || dataSize == 0)
	{
		if (data) SDL_free(data);
		return false;
	}

	outData.assign((const uint8_t*)data, (const uint8_t*)data + dataSize);
	SDL_free(data);
	return true;
#else
	// 桌面端：直接文件系统读取（保持现状）
	std::ifstream file(ResolveResourcePath(relPath), std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		return false;
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	outData.resize((size_t)size);
	file.read((char*)outData.data(), size);
	return file.good() || file.eof();
#endif
}

bool AssetManager::ResourceExists(const std::string& relPath)
{
#if GNX_OS_IOS || GNX_OS_ANDROID
	// SDL 无"文件存在"API，通过尝试打开判断
	SDL_RWops* rw = SDL_RWFromFile(relPath.c_str(), "rb");
	if (!rw)
	{
		return false;
	}
	SDL_RWclose(rw);
	return true;
#else
	std::ifstream f(ResolveResourcePath(relPath), std::ios::binary);
	return f.good();
#endif
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
	// 统一去掉扩展名作为缓存 key / name
	std::string baseName = texturePath;
	{
		size_t slash = baseName.find_last_of("/\\");
		if (slash != std::string::npos)
		{
			baseName = baseName.substr(slash + 1);
		}
		size_t ext = baseName.find(".texture");
		if (ext != std::string::npos)
		{
			baseName = baseName.substr(0, ext);
		}
	}

	const std::string resolvedPath = ResolveResourcePath(texturePath);
	auto pathEntry = mTexturesByPath.find(resolvedPath);
	TextureAsset* existing = pathEntry == mTexturesByPath.end() ? nullptr : pathEntry->second;
	if (existing)
	{
		existing->AddRef();
		return existing;
	}

	// 读取 .texture 完整文件内容（移动端走包内资源）
	std::vector<uint8_t> fileData;
	bool readOK = false;
#if GNX_OS_IOS || GNX_OS_ANDROID
	readOK = LoadResource(texturePath, fileData);
#else
	fileData = baselib::FileUtil::ReadBinaryFile(resolvedPath);
	readOK = !fileData.empty();
#endif

	if (!readOK)
	{
		std::cerr << "Failed to read texture asset file: " << texturePath << std::endl;
		return nullptr;
	}

	// 新建 TextureAsset 并加载（LoadFromMemory 解析 AssetFileHeader + TextureMessage）
	TextureAsset* texture = new TextureAsset();
	if (!texture->LoadFromMemory(fileData.data(), fileData.size()))
	{
		std::cerr << "Failed to parse texture asset: " << texturePath << std::endl;
		delete texture;
		return nullptr;
	}

#if GNX_OS_IOS || GNX_OS_ANDROID
	texture->SetAssetInfo(baseName, texturePath);
#else
	texture->SetAssetInfo(baseName, ResolveResourcePath(texturePath));
#endif

	// 添加到缓存（缓存持有引用）
	AddToCache(resolvedPath, texture);
	mTexturesByPath[resolvedPath] = texture;
	texture->AddRef();

	std::cout << "Loaded texture asset: " << texturePath << std::endl;
	return texture;
}



Asset* AssetManager::FindAsset(const std::string& guid)
{
	auto it = mAssets.find(guid);
	if (it != mAssets.end())
	{
		return it->second;
	}
	if (TextureAsset* texture = FindTexture(guid))
		return texture;
	if (ShaderAsset* shader = FindShader(guid))
		return shader;
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
	for (auto iterator = mAssets.begin(); iterator != mAssets.end();)
		iterator = iterator->second == asset ? mAssets.erase(iterator)
		                                    : std::next(iterator);

	if (asset->GetType() == AssetType::Texture)
	{
		TextureAsset* texture = static_cast<TextureAsset*>(asset);
		mTextures.erase(texture->GetName());
	}
	else if (asset->GetType() == AssetType::Shader)
	{
		ShaderAsset* shader = static_cast<ShaderAsset*>(asset);
		mShaders.erase(shader->GetName());
	}

	// 从GPU释放并卸载
	asset->ReleaseFromGPU();
	asset->Unload();
	for (auto iterator = mTexturesByPath.begin(); iterator != mTexturesByPath.end();)
		iterator = iterator->second == asset ? mTexturesByPath.erase(iterator) : std::next(iterator);
	for (auto iterator = mShadersByPath.begin(); iterator != mShadersByPath.end();)
		iterator = iterator->second == asset ? mShadersByPath.erase(iterator)
		                                    : std::next(iterator);

	std::cout << "Unloaded asset: " << guid << std::endl;
	delete asset;
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
	for (auto iterator = mRetiredAssets.begin(); iterator != mRetiredAssets.end();)
	{
		Asset* asset = *iterator;
		if (asset->GetRefCount() != 0)
		{
			++iterator;
			continue;
		}
		asset->ReleaseFromGPU();
		asset->Unload();
		delete asset;
		iterator = mRetiredAssets.erase(iterator);
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
	mShaders.clear();
	mTexturesByPath.clear();
	mShadersByPath.clear();
	for (Asset* asset : mRetiredAssets)
	{
		if (asset->GetRefCount() == 0)
		{
			asset->ReleaseFromGPU();
			asset->Unload();
			delete asset;
		}
	}
	mRetiredAssets.clear();

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

void AssetManager::AddToCache(const std::string& guid, Asset* asset)
{
	// 添加到主资源映射
	mAssets[guid] = asset;

	// 根据运行时类型添加到特定映射
	if (asset->GetType() == AssetType::Texture)
	{
		TextureAsset* texture = static_cast<TextureAsset*>(asset);
		mTextures[texture->GetName()] = texture;
	}
	else if (asset->GetType() == AssetType::Shader)
	{
		ShaderAsset* shader = static_cast<ShaderAsset*>(asset);
		mShaders[shader->GetName()] = shader;
	}
}

ShaderAsset* AssetManager::LoadShader(const std::string& filePath)
{
	if (!mInitialized)
	{
		std::cerr << "AssetManager not initialized!" << std::endl;
		return nullptr;
	}

	const std::string resolvedPath = ResolveResourcePath(filePath);
	auto byPath = mShadersByPath.find(resolvedPath);
	if (byPath != mShadersByPath.end())
	{
		byPath->second->AddRef();
		return byPath->second;
	}

	std::string fileName = resolvedPath;
	auto pos = fileName.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		fileName = fileName.substr(pos + 1);
	}

	ShaderAsset* shader = new ShaderAsset();
	if (!shader->LoadFromFile(resolvedPath))
	{
		std::cerr << "Failed to load shader: " << resolvedPath << std::endl;
		delete shader;
		return nullptr;
	}

	mAssets[resolvedPath] = shader;
	mShaders[fileName] = shader;
	mShadersByPath[resolvedPath] = shader;
	shader->AddRef();

	return shader;
}

ShaderAsset* AssetManager::FindShader(const std::string& name)
{
	// 只按完整文件名（含 format 后缀，如 GBufferPBR.spirv.gnxasset）精确查找。
	// 不同 format 是不同资源，不能去扩展名回退（防跨格式串缓存，漏洞15）。
	auto it = mShaders.find(name);
	if (it != mShaders.end())
	{
		return it->second;
	}

	return nullptr;
}

TextureAsset* AssetManager::LoadTextureByHash(uint64_t hash)
{
	if (!mInitialized)
	{
		std::cerr << "AssetManager not initialized!" << std::endl;
		return nullptr;
	}

	return LoadTexture(std::to_string(hash));
}

NS_ASSETMANAGER_END
