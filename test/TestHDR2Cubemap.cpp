#include "Runtime/AssetProcess/source/TextureProcess/EnvHdrProcess.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/ImageCodec/include/ImageEncoder.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include <iostream>

void PrintUsage(const char* programName)
{
	std::cout << "Usage: " << programName << " <input_hdr> <output_cubemap>" << std::endl;
	std::cout << std::endl;
	std::cout << "Arguments:" << std::endl;
	std::cout << "  input_hdr      Path to input HDR environment map (equirectangular projection)" << std::endl;
	std::cout << "  output_cubemap Path to output cubemap image (vertical cross format, PNG)" << std::endl;
	std::cout << std::endl;
	std::cout << "Example:" << std::endl;
	std::cout << "  " << programName << " environment.hdr cubemap_cross.png" << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Error: Invalid number of arguments." << std::endl;
		PrintUsage(argv[0]);
		return 1;
	}

	std::string inputFile = argv[1];
	std::string outputFile = argv[2];

	std::cout << "Input HDR:  " << inputFile << std::endl;
	std::cout << "Output PNG: " << outputFile << std::endl;

	// Decode HDR image
	imagecodec::VImage image;
	if (!imagecodec::ImageDecoder::DecodeFile(inputFile.c_str(), &image))
	{
		std::cerr << "Error: Failed to decode HDR file: " << inputFile << std::endl;
		return 1;
	}

	std::cout << "HDR image loaded: " << image.GetWidth() << "x" << image.GetHeight() << std::endl;

	// Convert equirectangular map to vertical cross cubemap
	imagecodec::VImagePtr resultImage = AssetProcess::ConvertEquirectangularMapToVerticalCross(&image);
	if (!resultImage)
	{
		std::cerr << "Error: Failed to convert to cubemap." << std::endl;
		return 1;
	}

	std::cout << "Cubemap generated: " << resultImage->GetWidth() << "x" << resultImage->GetHeight() << std::endl;

	// Convert from RGB32Float to RGBA8 with tone mapping
	imagecodec::VImagePtr result1 = std::make_shared<imagecodec::VImage>();
	result1->SetImageInfo(imagecodec::FORMAT_RGBA8, resultImage->GetWidth(), resultImage->GetHeight());
	result1->AllocPixels();

	float* pSrc = (float*)resultImage->GetImageData();
	uint8_t* pDst = (uint8_t*)result1->GetImageData();
	uint32_t offset = 0;

	for (uint32_t i = 0; i < resultImage->GetWidth() * resultImage->GetHeight(); i++)
	{
		float R = pSrc[offset + 0];
		float G = pSrc[offset + 1];
		float B = pSrc[offset + 2];

		// Calculate luminance using Rec.709 weights
		float L = 0.2126f * R + 0.7152f * G + 0.0722f * B;

		// Apply Reinhard tone mapping
		float Ld = L / (1.0f + L);

		// Scale RGB based on compressed luminance
		float scale = (L > 0.0f) ? (Ld / L) : 1.0f;
		R = R * scale;
		G = G * scale;
		B = B * scale;

		// Clamp to [0, 1]
		R = Clamp(R, 0.0f, 1.0f);
		G = Clamp(G, 0.0f, 1.0f);
		B = Clamp(B, 0.0f, 1.0f);

		// Convert to 8-bit
		pDst[i * 4 + 0] = (uint8_t)(R * 255.0f + 0.5f);
		pDst[i * 4 + 1] = (uint8_t)(G * 255.0f + 0.5f);
		pDst[i * 4 + 2] = (uint8_t)(B * 255.0f + 0.5f);
		pDst[i * 4 + 3] = 255;

		offset += 3;
	}

	// Encode output image
	if (!imagecodec::ImageEncoder::EncodeFile(outputFile.c_str(), *result1, imagecodec::ImageStoreFormat::kPNG_Format, 100))
	{
		std::cerr << "Error: Failed to encode output file: " << outputFile << std::endl;
		return 1;
	}

	std::cout << "Cubemap saved successfully: " << outputFile << std::endl;
	return 0;
}