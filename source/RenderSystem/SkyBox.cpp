//
//  SkyBox.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/6/12.
//

#include "SkyBox.h"
#include "ImageTextureUtil.h"
#include "ImageCodec/ImageDecoder.h"
#include "MathUtil/Vector3.h"
#include "MathUtil/Matrix4x4.h"
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
    std::vector<TextureDescriptor> desArray;
    for (auto iter : images)
    {
        if (nullptr == iter || iter->GetPixels() == nullptr)
        {
            return false;
        }
        TextureDescriptor textureDescriptor = ImageTextureUtil::getTextureDescriptor(*iter);
        desArray.push_back(textureDescriptor);
    }
    
//    mTextureCube = renderDevice->createTextureCubeWithDescriptor(desArray);
//    mTextureCube->setTextureData(kCubeFacePX, positive_x->GetImageSize(), positive_x->GetPixels());
//    mTextureCube->setTextureData(kCubeFaceNX, negative_x->GetImageSize(), negative_x->GetPixels());
//    mTextureCube->setTextureData(kCubeFacePY, positive_y->GetImageSize(), positive_y->GetPixels());
//    mTextureCube->setTextureData(kCubeFaceNY, negative_y->GetImageSize(), negative_y->GetPixels());
//    mTextureCube->setTextureData(kCubeFacePZ, positive_z->GetImageSize(), positive_z->GetPixels());
//    mTextureCube->setTextureData(kCubeFaceNZ, negative_z->GetImageSize(), negative_z->GetPixels());
    
    mTextureCube = LoadEquirectangularMap(getMediaDir() + "IBL/piazza_bologni_1k.hdr");
    
    SamplerDescriptor samplerDescriptor;
    mTextureSampler = renderDevice->createSamplerWithDescriptor(samplerDescriptor);
    
    initBuffers(renderDevice);
    
    ShaderAssetString shaderAssetString = LoadShaderAsset("Skybox");
    
    ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
    ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;
    
    //        FILE* fp1 = fopen("/Users/zhouxuguang/work/skybok.vert", "wb");
    //        fwrite(vertexShader, 1, strlen(vertexShader), fp1);
    //        fclose(fp1);
    //
    //        FILE* fp2 = fopen("/Users/zhouxuguang/work/skybok.frag", "wb");
    //        fwrite(fragmentShader, 1, strlen(fragmentShader), fp2);
    //        fclose(fp2);
    
    ShaderFunctionPtr vertShader = renderDevice->createShaderFunction(*vertexShader, ShaderStage_Vertex);
    ShaderFunctionPtr fragShader = renderDevice->createShaderFunction(*fragmentShader, ShaderStage_Fragment);
    GraphicsPipelineDescriptor graphicsPipelineDescriptor;
    graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = CompareFunctionLessThanOrEqual;
    graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = true;
    
    mPipeline = renderDevice->createGraphicsPipeline(graphicsPipelineDescriptor);
    mPipeline->attachVertexShader(vertShader);
    mPipeline->attachFragmentShader(fragShader);
    
    mRenderDevice = renderDevice;
    
    return true;
}

void SkyBox::Render(const RenderEncoderPtr &renderEncoder, UniformBufferPtr cameraUBO)
{
    if (!mRenderDevice)
    {
        return;
    }
    
    renderEncoder->setGraphicsPipeline(mPipeline);
    
    renderEncoder->setVertexBuffer(mVertexBuffer, 0, 0);
    renderEncoder->setFragmentTextureCubeAndSampler(mTextureCube, mTextureSampler, 0);
    renderEncoder->setVertexUniformBuffer(cameraUBO, 0);
    
    renderEncoder->drawPrimitves(PrimitiveMode_TRIANGLES, 0, 36);
}

void SkyBox::initBuffers(RenderDevicePtr renderDevice)
{
    // init vertex buffer object
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f,
         1.0f,  1.0f, -1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f,

         1.0f, -1.0f, -1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f,  1.0f, -1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f,

        -1.0f,  1.0f, -1.0f, 1.0f,
         1.0f,  1.0f, -1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f
    };
    mVertexBuffer = renderDevice->createVertexBufferWithBytes(skyboxVertices, sizeof(skyboxVertices), StorageModePrivate);
}

NS_RENDERSYSTEM_END
