//
//  SkyBox.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#include "SkyBox.h"
#include "ImageTextureUtil.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "ShaderAssetLoader.h"
#include "RenderEngine.h"

USING_NS_MATHUTIL

NS_RENDERSYSTEM_BEGIN

SkyBox::SkyBox()
{
}

SkyBox::~SkyBox()
{
    mTextureCube.reset();
}

SkyBox* SkyBox::create(RenderDevicePtr renderDevice, const char* positive_x, const char* negative_x, const char* positive_y, const char* negative_y,
                      const char* positive_z, const char* negative_z)
{
    if (nullptr == positive_x || nullptr == negative_x || nullptr == positive_y || nullptr == negative_y
        || nullptr == positive_z || nullptr == negative_z)
    {
        return nullptr;
    }
    
    VImagePtr positive_x_image = std::make_shared<VImage>();
    if (!ImageDecoder::DecodeFile(positive_x, positive_x_image.get()))
    {
        return nullptr;
    }
    
    VImagePtr negative_x_image = std::make_shared<VImage>();
    if (!ImageDecoder::DecodeFile(negative_x, negative_x_image.get()))
    {
        return nullptr;
    }
    
    VImagePtr positive_y_image = std::make_shared<VImage>();
    if (!ImageDecoder::DecodeFile(positive_y, positive_y_image.get()))
    {
        return nullptr;
    }
    
    VImagePtr negative_y_image = std::make_shared<VImage>();
    if (!ImageDecoder::DecodeFile(negative_y, negative_y_image.get()))
    {
        return nullptr;
    }
    
    VImagePtr positive_z_image = std::make_shared<VImage>();
    if (!ImageDecoder::DecodeFile(positive_z, positive_z_image.get()))
    {
        return nullptr;
    }
    
    VImagePtr negative_z_image = std::make_shared<VImage>();
    if (!ImageDecoder::DecodeFile(negative_z, negative_z_image.get()))
    {
        return nullptr;
    }
    
    SkyBox* skybox = new(std::nothrow)SkyBox();
    if (!skybox->init(renderDevice, positive_x_image, negative_x_image, positive_y_image, negative_y_image, positive_z_image, negative_z_image))
    {
        delete skybox;
        return nullptr;
    }
    
    return skybox;
}

SkyBox* SkyBox::create(RenderDevicePtr renderDevice, VImagePtr positive_x, VImagePtr negative_x, VImagePtr positive_y, VImagePtr negative_y,
                      VImagePtr positive_z, VImagePtr negative_z)
{
    if (nullptr == positive_x || nullptr == negative_x || nullptr == positive_y || nullptr == negative_y
        || nullptr == positive_z || nullptr == negative_z)
    {
        return nullptr;
    }
    
    SkyBox* skybox = new(std::nothrow)SkyBox();
    if (!skybox->init(renderDevice, positive_x, negative_x, positive_y, negative_y, positive_z, negative_z))
    {
        delete skybox;
        return nullptr;
    }
    
    return skybox;
}

SkyBox* SkyBox::createFromTexture(RenderDevicePtr renderDevice, RCTextureCubePtr textureCube)
{
    if (!renderDevice || !textureCube)
    {
        return nullptr;
    }

    SkyBox* skybox = new(std::nothrow)SkyBox();
    if (!skybox->initFromTexture(renderDevice, textureCube))
    {
        delete skybox;
        return nullptr;
    }

    return skybox;
}

void SkyBox::destroy(SkyBox* skybox)
{
    delete skybox;
}

bool SkyBox::init(RenderDevicePtr renderDevice, VImagePtr positive_x, VImagePtr negative_x, VImagePtr positive_y, VImagePtr negative_y,
                  VImagePtr positive_z, VImagePtr negative_z)
{
    if (nullptr == renderDevice)
    {
        return false;
    }
    std::vector<VImagePtr> images;
    images.push_back(positive_x);
    images.push_back(negative_x);
    images.push_back(positive_y);
    images.push_back(negative_y);
    images.push_back(positive_z);
    images.push_back(negative_z);
    std::vector<TextureDesc> desArray;
    for (auto iter : images)
    {
        if (nullptr == iter || iter->GetImageData() == nullptr)
        {
            return false;
        }
        TextureDesc textureDescriptor = ImageTextureUtil::getTextureDescriptor(*iter);
        desArray.push_back(textureDescriptor);
    }
    
    SamplerDesc samplerDescriptor;
    mTextureSampler = renderDevice->CreateSamplerWithDescriptor(samplerDescriptor);

    // 创建 Cubemap 纹理并上传 6 个面的数据
    TextureDesc& firstDesc = desArray[0];
    mTextureCube = renderDevice->CreateTextureCube(
        firstDesc.format, TextureUsage::TextureUsageShaderRead,
        firstDesc.width, firstDesc.height, 1);

    for (int i = 0; i < 6; ++i)
    {
        mTextureCube->ReplaceRegion(
            Rect2D(0, 0, desArray[i].width, desArray[i].height),
            0, i,
            (const uint8_t*)images[i]->GetImageData(),
            desArray[i].width * 4,
            desArray[i].width * desArray[i].height * 4);
    }

    initBuffers(renderDevice);
    
    ShaderAssetString shaderAssetString = LoadShaderAsset("Skybox");
    
    ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
    ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;

    GraphicsShaderPtr shader = renderDevice->CreateGraphicsShader(*vertexShader, *fragmentShader);

    GraphicsPipelineDesc graphicsPipelineDescriptor;
    graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetSkyboxDepthCompareFunc();
    graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = false;
    
    mPipeline = renderDevice->CreateGraphicsPipeline(graphicsPipelineDescriptor);
    mPipeline->AttachGraphicsShader(shader);
    
    mRenderDevice = renderDevice;

    return true;
}

bool SkyBox::initFromTexture(RenderDevicePtr renderDevice, RCTextureCubePtr textureCube)
{
    if (!renderDevice || !textureCube)
    {
        return false;
    }

    mTextureCube = textureCube;

    SamplerDesc samplerDescriptor;
    mTextureSampler = renderDevice->CreateSamplerWithDescriptor(samplerDescriptor);

    initBuffers(renderDevice);

    ShaderAssetString shaderAssetString = LoadShaderAsset("Skybox");

    ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
    ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;

    GraphicsShaderPtr shader = renderDevice->CreateGraphicsShader(*vertexShader, *fragmentShader);

    GraphicsPipelineDesc graphicsPipelineDescriptor;
    graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetSkyboxDepthCompareFunc();
    graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = false;

    mPipeline = renderDevice->CreateGraphicsPipeline(graphicsPipelineDescriptor);
    mPipeline->AttachGraphicsShader(shader);

    mRenderDevice = renderDevice;

    return true;
}

void SkyBox::Render(const RenderEncoderPtr &renderEncoder, UniformBufferPtr cameraUBO)
{
    if (!mRenderDevice)
    {
        return;
    }
    
    renderEncoder->SetGraphicsPipeline(mPipeline);
    
    renderEncoder->SetVertexBuffer(mVertexBuffer, 0, 0);
    renderEncoder->SetFragmentTextureAndSampler("gCubeMap", mTextureCube, mTextureSampler);
    renderEncoder->SetVertexUniformBuffer("cbPerCamera", cameraUBO);
    
    renderEncoder->DrawPrimitves(PrimitiveMode_TRIANGLES, 0, 36);
}

void SkyBox::initBuffers(RenderDevicePtr renderDevice)
{
    // init vertex buffer object
    float skyboxVertices[] = {
        // positions (float3, shader uses float3 position : POSITION)
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    mVertexBuffer = renderDevice->CreateVertexBufferWithBytes(skyboxVertices, sizeof(skyboxVertices), StorageModePrivate);
}

NS_RENDERSYSTEM_END
