#ifndef GNX_ENGINE_ASSET_IMPORTER_INCLUDE_SGMDFGNJ
#define GNX_ENGINE_ASSET_IMPORTER_INCLUDE_SGMDFGNJ

#include "AssetProcessDefine.h"

NS_ASSETPROCESS_BEGIN

//资产导入的总入口

class AssetImporter
{
public:
	AssetImporter();
	~AssetImporter();

	bool ImportFromFile(const std::string& fileName, const std::string& saveDir);

private:
	
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ASSET_IMPORTER_INCLUDE_SGMDFGNJ

