#include "EnvHdrProcess.h"

NS_ASSETPROCESS_BEGIN

using namespace mathutil;

template <typename T>
T clamp(T v, T a, T b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

static Vector3f FaceCoordsToXYZ(uint32_t i, uint32_t j, uint32_t faceID, uint32_t faceSize)
{
	const float A = 2.0f * float(i) / faceSize;
	const float B = 2.0f * float(j) / faceSize;

	if (faceID == 0) return Vector3f(-1.0f, A - 1.0f, B - 1.0f);      // +X
	if (faceID == 1) return Vector3f(A - 1.0f, -1.0f, 1.0f - B);      // -X
	if (faceID == 2) return Vector3f(1.0f, A - 1.0f, 1.0f - B);       // +Y
	if (faceID == 3) return Vector3f(1.0f - A, 1.0f, 1.0f - B);       // -Y
	if (faceID == 4) return Vector3f(B - 1.0f, A - 1.0f, 1.0f);       // +Z
	if (faceID == 5) return Vector3f(1.0f - B, A - 1.0f, -1.0f);      // -Z

	return Vector3f();
}

imagecodec::VImagePtr ConvertEquirectangularMapToVerticalCross(const imagecodec::VImage* envImage)
{
	if (!envImage) return nullptr;

	if (envImage->GetWidth() != envImage->GetHeight() * 2)
	{
		return nullptr;
	}

	// 计算cubemap的宽高等信息
	const uint32_t faceSize = envImage->GetWidth() / 4;

	const uint32_t w = faceSize * 3;
	const uint32_t h = faceSize * 4;

	imagecodec::VImagePtr result = std::make_shared<imagecodec::VImage>();
	result->SetImageInfo(envImage->GetFormat(), w, h);
	result->AllocPixels();
	memset(result->GetImageData(), 0, w * h * 4 * 3);

	// 各个面的图像在垂直交叉的cubemap中的偏移位置
	const Vector2i kFaceOffsets[] =
	{
		Vector2i(faceSize, faceSize * 3),
		Vector2i(0, faceSize),
		Vector2i(faceSize, faceSize),
		Vector2i(faceSize * 2, faceSize),
		Vector2i(faceSize, 0),
		Vector2i(faceSize, faceSize * 2)
	};

	const uint32_t clampW = envImage->GetWidth() - 1;
	const uint32_t clampH = envImage->GetHeight() - 1;

	for (uint32_t face = 0; face != 6; face ++)
	{
		for (uint32_t i = 0; i != faceSize; i ++)
		{
			for (uint32_t j = 0; j != faceSize; j ++)
			{
				const Vector3f P = FaceCoordsToXYZ(i, j, face, faceSize);
				const float R = hypot(P.x, P.y);
				const float theta = atan2(P.y, P.x);
				const float phi = atan2(P.z, R);
				//	float point source coordinates
				const float Uf = float(2.0f * faceSize * (theta + M_PI) / M_PI);
				const float Vf = float(2.0f * faceSize * (M_PI / 2.0f - phi) / M_PI);
				// 4-samples for bilinear interpolation
				const uint32_t U1 = clamp(uint32_t(floor(Uf)), 0u, clampW);
				const uint32_t V1 = clamp(uint32_t(floor(Vf)), 0u, clampH);
				const uint32_t U2 = clamp(U1 + 1, 0u, clampW);
				const uint32_t V2 = clamp(V1 + 1, 0u, clampH);
				// fractional part
				const float s = Uf - U1;
				const float t = Vf - V1;
				// fetch 4-samples
				const Vector4f A = envImage->GetPixel(U1, V1);
				const Vector4f B = envImage->GetPixel(U2, V1);
				const Vector4f C = envImage->GetPixel(U1, V2);
				const Vector4f D = envImage->GetPixel(U2, V2);
				// bilinear interpolation
				const Vector4f color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D * (s) * (t);
				result->SetPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
			}
		};
	}

	return result;
}

std::vector<imagecodec::VImagePtr> ConvertVerticalCrossToCubeMapFaces(const imagecodec::VImage* envImage)
{
	if (!envImage)
	{
		return {};
	}

	uint32_t width = envImage->GetWidth();
	uint32_t height = envImage->GetHeight();
	const uint32_t faceWidth = width / 3;
	const uint32_t faceHeight = height / 4;

	const uint8_t* src = (const uint8_t*)envImage->GetImageData();

	std::vector<imagecodec::VImagePtr> cubeMaps;

	/*
			------
			| +Y |
	   ----------------
	   | -X | -Z | +X |
	   ----------------
			| -Y |
			------
			| +Z |
			------
	*/

	const uint32_t pixelSize = envImage->GetBytesPerPixels();

	for (uint32_t face = 0; face != 6; ++face)
	{
		imagecodec::VImagePtr cubeMap = std::make_shared<imagecodec::VImage>();
		cubeMap->SetImageInfo(envImage->GetFormat(), faceWidth, faceHeight);
		cubeMap->AllocPixels();

		uint8_t* pImageData = cubeMap->GetImageData();

		for (uint32_t j = 0; j != faceHeight; ++j)
		{
			for (uint32_t i = 0; i != faceWidth; ++i)
			{
				uint32_t x = 0;
				uint32_t y = 0;

				switch (face)
				{
					// GL_TEXTURE_CUBE_MAP_POSITIVE_X
				case 0:
					x = i;
					y = faceHeight + j;
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
				case 1:
					x = 2 * faceWidth + i;
					y = 1 * faceHeight + j;
					break;

					// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
				case 2:
					x = 2 * faceWidth - (i + 1);
					y = 1 * faceHeight - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
				case 3:
					x = 2 * faceWidth - (i + 1);
					y = 3 * faceHeight - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
				case 4:
					x = 2 * faceWidth - (i + 1);
					y = height - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
				case 5:
					x = faceWidth + i;
					y = faceHeight + j;
					break;
				}

				memcpy(pImageData, src + (y * width + x) * pixelSize, pixelSize);

				pImageData += pixelSize;
			}
		}

		cubeMaps.push_back(cubeMap);
	}

	return cubeMaps;
}

NS_ASSETPROCESS_END