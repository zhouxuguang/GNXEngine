#include "RenderSystem/RenderEngine.h"
#include "AssetProcess/AssetImporter.h"
#include "BaseLib/ThreadPool.h"
#include "BaseLib/LogService.h"
#include "BaseLib/TimeCost.h"
#include "AssetProcess/ASTCCompressor.h"
#include "ImageCodec/ImageDecoder.h"

#include "ktx.h"

// little endian
struct astc_header
{
    uint8_t magic[4];
    uint8_t blockdim_x;
    uint8_t blockdim_y;
    uint8_t blockdim_z;
    uint8_t xsize[3];
    uint8_t ysize[3];            // x-size, y-size and z-size are given in texels;
    uint8_t zsize[3];            // block count is inferred
};

#define MAGIC_FILE_CONSTANT 0x5CA1AB13

void store_astc(uint8_t* pData, uint32_t dataSize, uint32_t width, uint32_t height, uint32_t block_width, uint32_t block_height, const char* filename)
{
    FILE* f = fopen(filename, "wb");

    astc_header file_header;

    uint32_t magic = MAGIC_FILE_CONSTANT;
    memcpy(file_header.magic, &magic, 4);
    file_header.blockdim_x = block_width;
    file_header.blockdim_y = block_height;
    file_header.blockdim_z = 1;

    int xsize = width;
    int ysize = height;
    int zsize = 1;

    memcpy(file_header.xsize, &xsize, 3);
    memcpy(file_header.ysize, &ysize, 3);
    memcpy(file_header.zsize, &zsize, 3);

    fwrite(&file_header, sizeof(astc_header), 1, f);
    fwrite(pData, dataSize, 1, f);

    fclose(f);
}

void testKTX()
{
	fs::path currentPath = getMediaDir();

	fs::path filePath = (currentPath / fs::path("backpack/diffuse.jpg")).lexically_normal();
	std::string imageFilePath = filePath.string();
    imagecodec::VImage image;
    imagecodec::ImageDecoder::DecodeFile(imageFilePath.c_str(), &image);
	// create a KTX2 texture for RGBA data
    ktxTextureCreateInfo createInfoKTX2 = {};
    createInfoKTX2.glInternalformat = 0x8058;
    createInfoKTX2.vkFormat = 37;
    createInfoKTX2.baseWidth = (uint32_t)image.GetWidth();
    createInfoKTX2.baseHeight = (uint32_t)image.GetHeight();
    createInfoKTX2.baseDepth = 1u;
    createInfoKTX2.numDimensions = 2u;
    createInfoKTX2.numLevels = 1;
    createInfoKTX2.numLayers = 1u;
    createInfoKTX2.numFaces = 1u;
    createInfoKTX2.generateMipmaps = KTX_FALSE;
    ktxTexture1* textureKTX2 = nullptr;
	ktxTexture1_Create(&createInfoKTX2, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX2);

	for (uint32_t i = 0; i != 1; ++i) 
    {
		size_t offset = 0;
		ktxTexture_GetImageOffset(ktxTexture(textureKTX2), i, 0, 0, &offset);

        uint8_t* pDst = ktxTexture_GetData(ktxTexture(textureKTX2)) + offset;
        memcpy(pDst, image.GetPixels(), image.GetWidth() * image.GetHeight() * 4);
	}

	ktxTexture_WriteToNamedFile(ktxTexture(textureKTX2), "image1.ktx");
	ktxTexture_Destroy(ktxTexture(textureKTX2));
}

int main(int argc, char* argv[])
{
	fs::path currentPath = getMediaDir();

    testKTX();

	int a = 100;
	LOG_INFO("BaseLib/LogService.h %d", a);

	fs::path filePath = (currentPath/fs::path("backpack/backpack.obj")).lexically_normal();
	std::string modelPath = filePath.string();

	AssetProcess::AssetImporter assetImporter;
	assetImporter.ImportFromFile(modelPath, filePath.parent_path().string());
    
    imagecodec::VImagePtr image = std::make_shared<imagecodec::VImage>();
    filePath = (currentPath/fs::path("backpack/diffuse.jpg")).lexically_normal();
    modelPath = filePath.string();
    imagecodec::ImageDecoder::DecodeFile(modelPath.c_str(), image.get());
    
    uint32_t block_width = 6;
    uint32_t block_height = 6;
    uint32_t xBlockCount = (image->GetWidth() + block_width - 1) / block_width;
    uint32_t yBlockCount = (image->GetHeight() + block_height - 1) / block_height;
    
    uint8_t* pOut = new uint8_t[xBlockCount * yBlockCount * 16];
    baselib::TimeCost cost;
    cost.Begin();
    AssetProcess::CompressASTC(pOut, image->GetPixels(), image->GetWidth(), image->GetHeight(), block_width, block_height, image->GetBytesPerRow());
    cost.End();
    
    uint64_t t1 = cost.GetCostTime();
    LOG_INFO("AssetProcess::CompressASTC cost time = %lf", t1 / 1000.0);
    
    cost.Begin();
    AssetProcess::CompressASTC_MT(pOut, image->GetPixels(), image->GetWidth(), image->GetHeight(), block_width, block_height, image->GetBytesPerRow());
    cost.End();
    t1 = cost.GetCostTime();
    LOG_INFO("AssetProcess::CompressASTC_MT cost time = %lf", t1 / 1000.0);
    
    store_astc(pOut, xBlockCount * yBlockCount * 16, image->GetWidth(), image->GetHeight(), block_width, block_height, "test.astc");
    delete [] pOut;

	return 0;
}
