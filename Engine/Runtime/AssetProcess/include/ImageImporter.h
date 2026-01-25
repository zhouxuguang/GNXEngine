#ifndef GNX_ENGINE_IMAGE_IMPORTER_JSDGJDFJ_INCLUDE
#define GNX_ENGINE_IMAGE_IMPORTER_JSDGJDFJ_INCLUDE

#include "AssetProcessDefine.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"


//USING_NS_RENDERSYSTEM

NS_ASSETPROCESS_BEGIN

// 图片加载

class ImageImporter
{
public:
	ImageImporter(const char* fileName, const std::string& saveDir);
	~ImageImporter();

	bool Load();

	// 从内存数据导入纹理（用于压缩的嵌入式纹理）
	static bool LoadFromMemory(const uint8_t* data, size_t size, const std::string& saveDir, const std::string& outputFileName);

	// 从原始像素数据导入纹理（用于未压缩的嵌入式纹理）
	static bool LoadFromRawPixels(const uint8_t* data, uint32_t width, uint32_t height, imagecodec::ImagePixelFormat format, const std::string& saveDir, const std::string& outputFileName);

private:
	std::string mFileName;
	std::string mSaveDir;
};

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_IMAGE_IMPORTER_JSDGJDFJ_INCLUDE

