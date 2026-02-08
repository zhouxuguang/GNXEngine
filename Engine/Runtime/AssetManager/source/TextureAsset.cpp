#include "TextureAsset.h"
#include "AssetFileHeader.h"
#include "Runtime/BaseLib/include/BaseLib.h"
#include <algorithm>
#include "TextureMessageUtil.h"


NS_ASSETMANAGER_BEGIN

// OpenGL 内部压缩格式枚举定义
// 参考: https://registry.khronos.org/OpenGL/index_gl.php
enum GLInternalFormat : uint32_t
{
    // S3TC (DXT) 压缩格式 (EXT_texture_compression_s3tc)
    GL_COMPRESSED_RGB_S3TC_DXT1_EXT          = 0x83F0,
    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT         = 0x83F1,
    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT         = 0x83F2,
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT         = 0x83F3,
    
    // BPTC 压缩格式 (ARB_texture_compression_bptc)
    GL_COMPRESSED_RGBA_BPTC_UNORM            = 0x8E8C,
    GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM      = 0x8E8D,
    GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT      = 0x8E8E,
    GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT    = 0x8E8F,
    
    // RGTC 压缩格式 (ARB_texture_compression_rgtc)
    GL_COMPRESSED_RED_RGTC1                  = 0x8DBB,
    GL_COMPRESSED_SIGNED_RED_RGTC1           = 0x8DBC,
    GL_COMPRESSED_RG_RGTC2                   = 0x8DBD,
    GL_COMPRESSED_SIGNED_RG_RGTC2            = 0x8DBE,
    
    // PVRTC 压缩格式 (IMG_texture_compression_pvrtc)
    GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG       = 0x8C00,
    GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG       = 0x8C01,
    GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG      = 0x8C02,
    GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG      = 0x8C03,
    
    // ETC/EAC 压缩格式 (OES_compressed_ETC1_RGB8_texture, ARB_compressed_texture_compression)
    GL_COMPRESSED_R11_EAC                    = 0x9270,
    GL_COMPRESSED_SIGNED_R11_EAC             = 0x9271,
    GL_COMPRESSED_RG11_EAC                   = 0x9272,
    GL_COMPRESSED_SIGNED_RG11_EAC            = 0x9273,
    
    // 浮点格式
    GL_RGBA32F                               = 0x8814
};

// 纹理的文件头 https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html
struct TextureDataHeader
{
	char identifier[12];
	uint32_t endianness;
	uint32_t glType;
	uint32_t glTypeSize;
	uint32_t glFormat;
	uint32_t glInternalFormat;
	uint32_t glBaseInternalFormat; // e.g. GL_RGBA, GL_BGRA, GL_RED
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t numberOfArrayElements;
	uint32_t numberOfFaces;
	uint32_t numberOfMipmapLevels;
	uint32_t bytesOfKeyValueData;
};

TextureAsset::TextureAsset() : 
	mIsOnGPU(false)
	, mGpuHandle(nullptr)
{
}

TextureAsset::~TextureAsset()
{
	Unload();
	ReleaseFromGPU();
}

AssetType TextureAsset::GetType() const
{
	return AssetType::Texture;
}

const std::string& TextureAsset::GetGUID() const
{
	return m_guid;
}

const std::string& TextureAsset::GetName() const
{
	return m_name;
}

const std::string& TextureAsset::GetFilePath() const
{
	return m_filePath;
}

bool TextureAsset::Load()
{

	SetState(AssetState::Loaded);
	return true;
}

void TextureAsset::Unload()
{

	SetState(AssetState::Unloaded);
}

bool TextureAsset::Reload()
{
	Unload();
	return Load();
}

bool TextureAsset::IsLoaded() const
{
	return GetState() == AssetState::Loaded ||
	       GetState() == AssetState::Uploading ||
	       GetState() == AssetState::Ready;
}

bool TextureAsset::UploadToGPU()
{
	if (!IsLoaded())
	{
		return false;
	}

	// TODO: 调用RenderSystem上传纹理到GPU
	// 这里需要RenderSystem的支持

	mIsOnGPU = true;
	SetState(AssetState::Ready);
	return true;
}

void TextureAsset::ReleaseFromGPU()
{
	if (!mIsOnGPU)
	{
		return;
	}

	// TODO: 调用RenderSystem从GPU释放纹理
	// 这里需要RenderSystem的支持

	mIsOnGPU = false;
}

bool TextureAsset::IsOnGPU() const
{
	return mIsOnGPU;
}

uint32_t TextureAsset::GetWidth() const
{
	return mWidth;
}

uint32_t TextureAsset::GetHeight() const
{
	return mHeight;
}

uint32_t TextureAsset::GetDepth() const
{
	return mDepth;
}

uint32_t TextureAsset::GetMipLevels() const
{
	return mMipLevels;
}

uint32_t TextureAsset::GetArrayLayers() const
{
	return mArrayLayers;
}

uint32_t TextureAsset::GetBytesPerPixel() const
{
	return 0;
}

const uint8_t* TextureAsset::GetData() const
{
	return mTextureData.data();
}

uint32_t TextureAsset::GetDataSize() const
{
	return (uint32_t)mTextureData.size();
}

RenderCore::TextureFormat TextureAsset::GetFormat() const
{
    return mTextureFormat;
}

RenderCore::TextureType TextureAsset::GetTextureType() const
{
    return mTextureType;
}

bool TextureAsset::LoadFromFile(const std::string& filePath)
{
	ByteVector imageData = baselib::FileUtil::ReadBinaryFile(filePath);
	if (imageData.empty())
	{
		return false;
	}

	return LoadFromMemory(imageData.data(), imageData.size());
}

bool TextureAsset::LoadFromMemory(const void* pData, size_t dataSize)
{
	if (!pData || 0 == dataSize)
	{
		return false;
	}

	const uint8_t* pPBData = (const uint8_t*)pData;
	size_t assetHeader = sizeof(AssetFileHeader);

	ByteVector packedImageData;
	bool suc = TextureMessageUtil::DecodeTextureMessage(pPBData + assetHeader, dataSize - assetHeader, packedImageData);
	if (!suc)
	{
		return false;
	}

	// 解析元数据
	ParseMeta(packedImageData);

	size_t textureDataSize = packedImageData.size() - sizeof(TextureDataHeader);

	mTextureData.resize(textureDataSize);
	memcpy(mTextureData.data(), packedImageData.data() + sizeof(TextureDataHeader), textureDataSize);

	return true;
}

// 将 OpenGL 内部格式转换为引擎 TextureFormat
static RenderCore::TextureFormat ConvertGLFormatToEngineFormat(uint32_t glInternalFormat)
{
    switch (glInternalFormat)
    {
        // S3TC (DXT) 压缩格式
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return RenderCore::kTexFormatDXT1_RGB;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return RenderCore::kTexFormatDXT1_SRGB;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            return RenderCore::kTexFormatDXT3_RGB;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return RenderCore::kTexFormatDXT3_SRGB;
        
        // BPTC 压缩格式 (BC7, BC6H)
        case GL_COMPRESSED_RGBA_BPTC_UNORM:
            return RenderCore::kTexFormatBC7_RGB;
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return RenderCore::kTexFormatBC7_SRGB;
        case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            return RenderCore::kTexFormatBC6H;
        
        // RGTC 压缩格式
        case GL_COMPRESSED_RED_RGTC1:
            return RenderCore::kTexFormatEAC_R;
        case GL_COMPRESSED_SIGNED_RED_RGTC1:
            return RenderCore::kTexFormatEAC_R_SIGNED;
        case GL_COMPRESSED_RG_RGTC2:
            return RenderCore::kTexFormatEAC_RG;
        case GL_COMPRESSED_SIGNED_RG_RGTC2:
            return RenderCore::kTexFormatEAC_RG_SIGNED;
        
        // PVRTC 压缩格式
        case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
            return RenderCore::kTexFormatPVRTC_RGB4;
        case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
            return RenderCore::kTexFormatPVRTC_RGB2;
        case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
            return RenderCore::kTexFormatPVRTC_RGBA4;
        case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
            return RenderCore::kTexFormatPVRTC_RGBA2;
        
        // EAC 压缩格式
        case GL_COMPRESSED_R11_EAC:
            return RenderCore::kTexFormatEAC_R;
        case GL_COMPRESSED_SIGNED_R11_EAC:
            return RenderCore::kTexFormatEAC_R_SIGNED;
        case GL_COMPRESSED_RG11_EAC:
            return RenderCore::kTexFormatEAC_RG;
        case GL_COMPRESSED_SIGNED_RG11_EAC:
            return RenderCore::kTexFormatEAC_RG_SIGNED;
        
        // 浮点格式
        case GL_RGBA32F:
            return RenderCore::kTexFormatRGBA32Float;
        
        // 未知格式
        default:
            return RenderCore::kTexFormatInvalid;
    }
}

void TextureAsset::ParseMeta(const ByteVector& binData)
{
	TextureDataHeader* textureHeader = (TextureDataHeader*)binData.data();

	mWidth = textureHeader->pixelWidth;
	mHeight = textureHeader->pixelHeight;
	mMipLevels = textureHeader->numberOfMipmapLevels;
	mArrayLayers = textureHeader->numberOfArrayElements;

    // 转换 OpenGL 内部格式到引擎格式
    mTextureFormat = ConvertGLFormatToEngineFormat(textureHeader->glInternalFormat);

	mTextureType = RenderCore::TextureType_2D;
	if (textureHeader->pixelDepth > 0)
	{
		mTextureType = RenderCore::TextureType_3D;
		mDepth = textureHeader->pixelDepth;
	}
	else if (textureHeader->numberOfFaces == 6)
	{
		mTextureType = RenderCore::TextureType_CUBE;
	}
	else if (textureHeader->numberOfArrayElements > 0)
	{
		mTextureType = RenderCore::TextureType_2D_ARRAY;
		mArrayLayers = textureHeader->numberOfArrayElements;
	}
}

NS_ASSETMANAGER_END
