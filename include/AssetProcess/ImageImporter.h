#ifndef GNX_ENGINE_IMAGE_IMPORTER_JSDGJDFJ_INCLUDE
#define GNX_ENGINE_IMAGE_IMPORTER_JSDGJDFJ_INCLUDE

#include "AssetProcessDefine.h"
#include "ImageCodec/ImageDecoder.h"


//USING_NS_RENDERSYSTEM

NS_ASSETPROCESS_BEGIN

// 图片加载

class ImageImporter
{
public:
	ImageImporter(const char* fileName, const std::string& saveDir);
	~ImageImporter();

	bool Load();

private:
	std::string fileName;
	std::string mSaveDir;
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_IMAGE_IMPORTER_JSDGJDFJ_INCLUDE

