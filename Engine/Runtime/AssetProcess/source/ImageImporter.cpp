#include "ImageImporter.h"
#include "AssetImporter.h"
#include "ktx.h"
#include "Runtime/ImageCodec/include/ImageUtil.h"
#include "TextureProcess/stb_image_resize2.h"
#include "DXTCompressor.h"
#include "PVRCompressor.h"
#include "Runtime/BaseLib/include/AlignedMalloc.h"

NS_ASSETPROCESS_BEGIN

struct KTXFormat
{
	uint32_t glInternalformat;
	uint32_t vkFormat;
	stbir_pixel_layout stbLayout;
    stbir_datatype stbDatatype;
    stbir_edge stbEdge;
    stbir_filter stbFilter;
};

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

static const uint32_t VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000;
static const uint32_t VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001;
static const uint32_t VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002;
static const uint32_t VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003;
static const uint32_t VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004;
static const uint32_t VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005;
static const uint32_t VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006;
static const uint32_t VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007;


#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                      0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                     0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                     0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                     0x83F3

#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT                     0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT               0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT               0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT               0x8C4F

#define GL_COMPRESSED_RED_RGTC1                              0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1                       0x8DBC
#define GL_COMPRESSED_RG_RGTC2                               0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2                        0x8DBE

#define GL_COMPRESSED_RGBA_BPTC_UNORM                        0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM                  0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT                  0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT                0x8E8F

#define GL_RGBA32F 0x8814

#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG                   0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG                   0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG                  0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG                  0x8C03
#define GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT                  0x8A54
#define GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT                  0x8A55
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT            0x8A56
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT            0x8A57


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
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
		break;
	case imagecodec::FORMAT_GRAY8_ALPHA8:
		ktxFormat.glInternalformat = GL_COMPRESSED_RG_RGTC2;
		ktxFormat.vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
		ktxFormat.stbLayout = STBIR_2CHANNEL;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
		break;
	case imagecodec::FORMAT_RGBA8:
		ktxFormat.glInternalformat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		ktxFormat.vkFormat = VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
		ktxFormat.stbLayout = STBIR_RGBA;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
		break;
	case imagecodec::FORMAT_RGB8:
		ktxFormat.glInternalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		ktxFormat.vkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		ktxFormat.stbLayout = STBIR_RGB;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
		break;
	case imagecodec::FORMAT_SRGB8_ALPHA8:
		ktxFormat.glInternalformat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
		ktxFormat.vkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
		ktxFormat.stbLayout = STBIR_RGBA;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8_SRGB_ALPHA;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
		break;
	case imagecodec::FORMAT_SRGB8:
		ktxFormat.glInternalformat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
		ktxFormat.vkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		ktxFormat.stbLayout = STBIR_RGB;
        ktxFormat.stbDatatype = STBIR_TYPE_UINT8_SRGB;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
		break;
	case imagecodec::FORMAT_RGBA32Float:
		ktxFormat.glInternalformat = GL_RGBA32F;
		ktxFormat.vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        ktxFormat.stbDatatype = STBIR_TYPE_FLOAT;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
		break;
	case imagecodec::FORMAT_RGB32Float:
		ktxFormat.glInternalformat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
		ktxFormat.vkFormat = VK_FORMAT_BC6H_UFLOAT_BLOCK;
        ktxFormat.stbDatatype = STBIR_TYPE_FLOAT;
        ktxFormat.stbEdge = STBIR_EDGE_CLAMP;
        ktxFormat.stbFilter = STBIR_FILTER_MITCHELL;
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
	else if (vkFormat == VK_FORMAT_BC7_UNORM_BLOCK || vkFormat == VK_FORMAT_BC7_SRGB_BLOCK)
	{
		CompressBC7(pDest, imageData, width, height, width * 4);
	}
    else if (vkFormat == VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG || vkFormat == VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)
    {
        CompressPVRRGBA4Bpp(pDest, imageData, width, height);
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
	uint8_t* pFormatData = (uint8_t*)baselib::AlignedMalloc(width * height * image->GetBytesPerPixels() * 2, 64);

	for (uint32_t i = 0; i != numMipLevels; ++i)
	{
		size_t offset = 0;
		ktxTexture_GetImageOffset(ktxTexture(textureKTX1), i, 0, 0, &offset);

        stbir_resize((const unsigned char*)image->GetPixels(), width, height, 0, pTmpData, w, h, 0,
                     ktxFormat.stbLayout, ktxFormat.stbDatatype, ktxFormat.stbEdge, ktxFormat.stbFilter);

		uint8_t* pDestImage = pTmpData;

		if (image->GetFormat() == imagecodec::FORMAT_RGB8 || image->GetFormat() == imagecodec::FORMAT_SRGB8)
		{
			uint32_t pixelCount = w * h;
			for (uint32_t i = 0; i < pixelCount; i++)
			{
				pFormatData[i * 4 + 0] = pTmpData[i * 3 + 0];
				pFormatData[i * 4 + 1] = pTmpData[i * 3 + 1];
				pFormatData[i * 4 + 2] = pTmpData[i * 3 + 2];
				pFormatData[i * 4 + 3] = 255;
			}
			pDestImage = pFormatData;
		}

		CompressTexture(pDestImage, w, h, ktxTexture_GetData(ktxTexture(textureKTX1)) + offset, ktxFormat.vkFormat);

		h = h > 1 ? h >> 1 : 1;
		w = w > 1 ? w >> 1 : 1;
	}

	baselib::AlignedFree(pTmpData);
	baselib::AlignedFree(pFormatData);

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
