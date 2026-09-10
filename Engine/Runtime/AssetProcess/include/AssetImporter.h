#ifndef GNX_ENGINE_ASSET_IMPORTER_INCLUDE_SGMDFGNJ
#define GNX_ENGINE_ASSET_IMPORTER_INCLUDE_SGMDFGNJ

#include "AssetProcessDefine.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <string>
#include <vector>

NS_ASSETPROCESS_BEGIN

//资产导入的总入口

struct AssetImportOutcome
{
	bool success = false;
	std::string sourceFile;
	std::vector<std::string> generatedAssets;
	std::string errorCode;
	std::string errorMessage;
};

class ASSET_PROCESS_API AssetImporter
{
public:
	AssetImporter();
	~AssetImporter();

	bool ImportFromFile(const std::string& fileName, const std::string& saveDir);
	AssetImportOutcome ImportFromFileDetailed(const std::string& fileName,
		const std::string& saveDir, const std::string& projectRoot);

private:
	
};

baselib::NXGUID CreateGUIDFromBinaryData(const uint8_t* data, size_t size);

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_ASSET_IMPORTER_INCLUDE_SGMDFGNJ

