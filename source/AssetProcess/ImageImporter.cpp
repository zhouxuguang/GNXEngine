#include "ImageImporter.h"

NS_ASSETPROCESS_BEGIN

ImageImporter::ImageImporter(const char* fileName, const std::string& saveDir)
{
	this->fileName = fileName;
	mSaveDir = saveDir;
}

ImageImporter::~ImageImporter()
{
}

bool ImageImporter::Load()
{
	imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
	bool result = imagecodec::ImageDecoder::DecodeFile(fileName.c_str(), image.get());
	if (!result)
	{
		return result;
	}

	//生成ktx2的非压缩格式以及保存一些元数据

	//保存文件
}

NS_ASSETPROCESS_END