#ifndef GNX_ENGINE_ASSIMP_ASSET_IMPORTER_INCLUDE
#define GNX_ENGINE_ASSIMP_ASSET_IMPORTER_INCLUDE

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/cimport.h>
#include "AssetProcessDefine.h"
#include "AssetImporter.h"

NS_ASSETPROCESS_BEGIN

class AssimpAssetImporter
{
public:
	AssimpAssetImporter();
	~AssimpAssetImporter();

	bool ImportFromFile(const std::string& fileName, const std::string& saveDir);

private:
	//
};



NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ASSIMP_ASSET_IMPORTER_INCLUDE
