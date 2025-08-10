#include "ImageImporter.h"
#include "AssetImporter.h"
#include "ktx.h"
#include "imagecodec/ImageUtil.h"
#include "TextureProcess/stb_image_resize2.h"
#include "DXTCompressor.h"
#include "BaseLib/AlignedMalloc.h"

NS_ASSETPROCESS_BEGIN

struct KTXFormat
{
	uint32_t glInternalformat;
	uint32_t vkFormat;
	stbir_pixel_layout stbLayout;
};

//FORMAT_GRAY8 = 0,            //gray
//FORMAT_GRAY8_ALPHA8 = 1,     //GRAY AND ALPHA
//FORMAT_RGBA8 = 2,            //RGBA32位
//FORMAT_RGB8 = 3,             //RGB
//FORMAT_RGBA4444 = 4,
//FORMAT_RGB5A1 = 5,
//FORMAT_R5G6B5 = 6,
//
//FORMAT_SRGB8_ALPHA8 = 7,             //sRGB_alpha
//FORMAT_SRGB8 = 8,             //sRGB
//
//FORMAT_RGBA32Float = 9,            //RGBA32位float
//FORMAT_RGB32Float = 10,             //RGB float

static const uint32_t VK_FORMAT_R32G32B32A32_SFLOAT = 109;
static const uint32_t VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131;
static const uint32_t VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132;
static const uint32_t VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133;
static const uint32_t VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134;
static const uint32_t VK_FORMAT_BC2_UNORM_BLOCK = 135;
static const uint32_t VK_FORMAT_BC2_SRGB_BLOCK = 136;
static const uint32_t VK_FORMAT_BC3_UNORM_BLOCK = 137;
static const uint32_t VK_FORMAT_BC3_SRGB_BLOCK = 138;
static const uint32_t VK_FORMAT_BC4_UNORM_BLOCK = 139;
static const uint32_t VK_FORMAT_BC4_SNORM_BLOCK = 140;
static const uint32_t VK_FORMAT_BC5_UNORM_BLOCK = 141;
static const uint32_t VK_FORMAT_BC5_SNORM_BLOCK = 142;
static const uint32_t VK_FORMAT_BC6H_UFLOAT_BLOCK = 143;
static const uint32_t VK_FORMAT_BC6H_SFLOAT_BLOCK = 144;
static const uint32_t VK_FORMAT_BC7_UNORM_BLOCK = 145;
static const uint32_t VK_FORMAT_BC7_SRGB_BLOCK = 146;


#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F

#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE

#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F

#define GL_RGBA32F 0x8814


// 创建导入的格式信息
KTXFormat CreateImportedKTXFormat(uint32_t imageFormat)
{
	KTXFormat ktxFormat = {};
#if TARGET_X86_64
	switch (imageFormat)
	{
	case imagecodec::FORMAT_GRAY8:
		ktxFormat.glInternalformat = GL_COMPRESSED_RED_RGTC1;
		ktxFormat.vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
		ktxFormat.stbLayout = STBIR_1CHANNEL;
		break;
	case imagecodec::FORMAT_GRAY8_ALPHA8:
		ktxFormat.glInternalformat = GL_COMPRESSED_RG_RGTC2;
		ktxFormat.vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
		ktxFormat.stbLayout = STBIR_2CHANNEL;
		break;
	case imagecodec::FORMAT_RGBA8:
		ktxFormat.glInternalformat = GL_COMPRESSED_RGBA_BPTC_UNORM;
		ktxFormat.vkFormat = VK_FORMAT_BC7_UNORM_BLOCK;
		ktxFormat.stbLayout = STBIR_RGBA;
		break;
	case imagecodec::FORMAT_RGB8:
		ktxFormat.glInternalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		ktxFormat.vkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		ktxFormat.stbLayout = STBIR_RGB;
		break;
	case imagecodec::FORMAT_SRGB8_ALPHA8:
		ktxFormat.glInternalformat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
		ktxFormat.vkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
		ktxFormat.stbLayout = STBIR_RGBA;
		break;
	case imagecodec::FORMAT_SRGB8:
		ktxFormat.glInternalformat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
		ktxFormat.vkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		ktxFormat.stbLayout = STBIR_RGB;
		break;
	case imagecodec::FORMAT_RGBA32Float:
		ktxFormat.glInternalformat = GL_RGBA32F;
		ktxFormat.vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case imagecodec::FORMAT_RGB32Float:
		ktxFormat.glInternalformat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
		ktxFormat.vkFormat = VK_FORMAT_BC6H_UFLOAT_BLOCK;
		break;
	}
#endif

	return ktxFormat;
}

static void CompressTexture(const uint8_t* imageData, uint32_t width, uint32_t height, uint8_t* pDest, uint32_t vkFormat)
{
	if (vkFormat == VK_FORMAT_BC1_RGB_UNORM_BLOCK || vkFormat == VK_FORMAT_BC1_RGB_SRGB_BLOCK)
	{
		CompressDXT1(pDest, imageData, width, height, width * 4);
	}
	if (vkFormat == VK_FORMAT_BC7_UNORM_BLOCK || vkFormat == VK_FORMAT_BC7_SRGB_BLOCK)
	{
		CompressBC7(pDest, imageData, width, height, width * 4);
	}
}

std::vector<uint8_t> CreateKTXFormatData(imagecodec::VImagePtr image, bool generateMipmap, const std::string & guidStr)
{
	KTXFormat ktxFormat = CreateImportedKTXFormat(image->GetFormat());

	uint32_t width = image->GetWidth();
	uint32_t height = image->GetHeight();

	uint32_t numMipLevels = 1;
	if (generateMipmap)
	{
		numMipLevels = imagecodec::ImageUtil::CalcNumMipLevels(width, height);
	}

	ktxTextureCreateInfo createInfoKTX = {};
	createInfoKTX.glInternalformat = ktxFormat.glInternalformat;
	createInfoKTX.vkFormat = ktxFormat.vkFormat;
	createInfoKTX.baseWidth = width;
	createInfoKTX.baseHeight = height;
	createInfoKTX.baseDepth = 1u;
	createInfoKTX.numDimensions = 2u;
	createInfoKTX.numLevels = numMipLevels;
	createInfoKTX.numLayers = 1u;
	createInfoKTX.numFaces = 1u;
	createInfoKTX.generateMipmaps = KTX_FALSE;
	ktxTexture1* textureKTX1 = nullptr;
	ktxTexture1_Create(&createInfoKTX, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX1);

	uint32_t w = width;
	uint32_t h = height;

	uint8_t* pTmpData = (uint8_t*)baselib::AlignedMalloc(width * height * image->GetBytesPerPixels() * 2, 64);

	for (uint32_t i = 0; i != numMipLevels; ++i)
	{
		size_t offset = 0;
		ktxTexture_GetImageOffset(ktxTexture(textureKTX1), i, 0, 0, &offset);

		stbir_resize_uint8_linear(
			(const unsigned char*)image->GetPixels(), width, height, 0, pTmpData, w, h, 0, ktxFormat.stbLayout);
		CompressTexture(pTmpData, w, h, ktxTexture_GetData(ktxTexture(textureKTX1)) + offset, ktxFormat.vkFormat);

		h = h > 1 ? h >> 1 : 1;
		w = w > 1 ? w >> 1 : 1;
	}

	baselib::AlignedFree(pTmpData);


	//std::string fileName = guidStr + ".ktx";
	ktxTexture_WriteToNamedFile(ktxTexture(textureKTX1), guidStr.c_str());
	ktxTexture_Destroy(ktxTexture(textureKTX1));

	return std::vector<uint8_t>();
}

ImageImporter::ImageImporter(const char* fileName, const std::string& saveDir)
{
	this->mFileName = fileName;
	mSaveDir = saveDir;
}

ImageImporter::~ImageImporter()
{
}

bool ImageImporter::Load()
{
	std::vector<uint8_t> data = baselib::FileUtil::ReadBinaryFile(mFileName);
	if (data.empty())
	{
		return false;
	}

	// 计算GUID
	baselib::NXGUID guid = CreateGUIDFromBinaryData(data.data(), data.size());
	std::string guidStr = baselib::GUIDToString(guid);

	imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
	bool result = imagecodec::ImageDecoder::DecodeMemory(data.data(), data.size(), image.get());
	if (!result)
	{
		return result;
	}

	fs::path currentPath = mSaveDir;
	std::string fileName = guidStr + ".ktx";

	//生成ktx的压缩格式以及保存一些元数据
	std::vector<uint8_t> ktxData = CreateKTXFormatData(image, false, (currentPath / fileName).string());

	//保存文件
}

NS_ASSETPROCESS_END