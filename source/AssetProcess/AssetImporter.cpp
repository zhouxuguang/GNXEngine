#include "AssetImporter.h"
#include "AssimpAssetImporter.h"
#include "ImageImporter.h"

NS_ASSETPROCESS_BEGIN

static bool HasExtension(const fs::path& filePath, const std::string& ext) 
{
	return filePath.extension() == "." + ext;
}

AssetImporter::AssetImporter()
{
}

AssetImporter::~AssetImporter()
{
}

bool AssetImporter::ImportFromFile(const std::string& fileName, const std::string& saveDir)
{
	if (fileName.empty() || saveDir.empty())
	{
		return false;
	}

	// 这里通过后缀判断不同类型的资产文件
	fs::path filePath = fileName;
	if (HasExtension(filePath, "bmp") || 
		HasExtension(filePath, "tga") || 
		HasExtension(filePath, "jpeg") ||
		HasExtension(filePath, "jpg") ||
		HasExtension(filePath, "png") ||
		HasExtension(filePath, "hdr") ||
		HasExtension(filePath, "webp"))
	{
		//加载和处理图像
		ImageImporter imageImporter(fileName.c_str(), saveDir);
		return imageImporter.Load();
	}

	else if (HasExtension(filePath, "obj") ||
		HasExtension(filePath, "fbx") ||
		HasExtension(filePath, "gltf") ||
		HasExtension(filePath, "glb") ||
		HasExtension(filePath, "3ds"))
	{
		AssimpAssetImporter assimpAssetImporter;
		return assimpAssetImporter.ImportFromFile(fileName, saveDir);
	}
}

baselib::GUID CreateGUIDFromBinaryData(const uint8_t* data, size_t size)
{
	baselib::SHA256 sha;
	sha.update(data, size);
	std::array<uint8_t, 32> digest = sha.digest();

	return baselib::CreateGUIDFromBytes(digest.data());
}

NS_ASSETPROCESS_END