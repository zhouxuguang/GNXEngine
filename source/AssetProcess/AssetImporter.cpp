#include "AssetImporter.h"
#include "AssimpAssetImporter.h"

NS_ASSETPROCESS_BEGIN

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

	AssimpAssetImporter assimpAssetImporter;
	return assimpAssetImporter.ImportFromFile(fileName, saveDir);
}

NS_ASSETPROCESS_END