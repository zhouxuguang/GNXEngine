//
//  TestComputeShader.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/12.
//

#include "TestComputeShader.hpp"
#include "ShaderAssetLoader.h"
#include "RenderCore/RenderDevice.h"
#include "RenderEngine.h"
#include "ImageCodec/ImageDecoder.h"
#include "ImageTextureUtil.h"
#include <assert.h>

using namespace imagecodec;

using namespace RenderSystem;

void TestADD()
{
    ShaderAssetString shaderAssetString = LoadShaderAsset("TestADD");
    
    const char* computeShader = nullptr;
    if (getRenderDevice()->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        computeShader = shaderAssetString.metalShader.computeShaderStr.c_str();
    }
    
    FILE* fp1 = fopen("/Users/zhouxuguang/work/TestADD.metal", "wb");
    fwrite(computeShader, 1, strlen(computeShader), fp1);
    fclose(fp1);
    
    int count = 1 << 26;
    float *data_a = new float[count];
    for (int i = 0; i < count; i ++)
    {
        data_a[i] = i + 1;
    }
    ComputeBufferPtr buffer1 = getRenderDevice()->createComputeBuffer(data_a, count * 4, StorageModePrivate);
    
    float *data_b = new float[count];
    for (int i = 0; i < count; i ++)
    {
        data_b[i] = (i + 1) * 2;
    }
    ComputeBufferPtr buffer2 = getRenderDevice()->createComputeBuffer(data_b, count * 4, StorageModePrivate);
    
    ComputeBufferPtr buffer3 = getRenderDevice()->createComputeBuffer(data_b, count * 4, StorageModeShared);
    
    ComputePipelinePtr computePipeline = getRenderDevice()->createComputePipeline(computeShader);
    
    CommandBufferPtr command = getRenderDevice()->createCommandBuffer();
    
    ComputeEncoderPtr computeEncoder = command->createComputeEncoder();
    computeEncoder->SetComputePipeline(computePipeline);
    computeEncoder->SetBuffer(buffer1, 0);
    computeEncoder->SetBuffer(buffer2, 1);
    computeEncoder->SetBuffer(buffer3, 2);
    
    uint32_t x, y ,z;
    computePipeline->GetThreadGroupSizes(x, y, z);
    
    int groupX = (count + x - 1) / x;
    
    computeEncoder->Dispatch(groupX, 1, 1);
    
    computeEncoder->EndEncode();
    command->waitUntilCompleted();
    
    //检查结果
    float* a = data_a;
    float* b = data_b;
    float* result = (float*)buffer3->mapBufferData();

    for (unsigned long index = 0; index < count; index++)
    {
        if (result[index] != (a[index] + b[index]))
        {
            printf("Compute ERROR: index=%lu result=%g vs %g=a+b\n",
                   index, result[index], a[index] + b[index]);
            assert(result[index] == (a[index] + b[index]));
        }
    }
    printf("Compute results as expected\n");
    buffer3->unmapBufferData(result);
    
    delete [] data_a;
    delete [] data_b;
}

ComputePipelinePtr initTestimageGray()
{
    ShaderAssetString shaderAssetString = LoadShaderAsset("TestImageGray");
    
    const char* computeShader = nullptr;
    if (getRenderDevice()->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        computeShader = shaderAssetString.metalShader.computeShaderStr.c_str();
    }
    
    FILE* fp1 = fopen("/Users/zhouxuguang/work/TestImageGray.metal", "wb");
    fwrite(computeShader, 1, strlen(computeShader), fp1);
    fclose(fp1);
    
    ComputePipelinePtr computePipeline = getRenderDevice()->createComputePipeline(computeShader);
    
    return computePipeline;
}

void testImageGrayDraw(ComputeEncoderPtr computeEncoder, ComputePipelinePtr computePipeline,
                       RenderTexturePtr inputTexture, RenderTexturePtr outputTexture)
{
    computeEncoder->SetComputePipeline(computePipeline);
    computeEncoder->SetTexture(inputTexture, 0, 0);
    computeEncoder->SetTexture(outputTexture, 0, 1);
    
    uint32_t x, y ,z;
    computePipeline->GetThreadGroupSizes(x, y, z);
    
    int groupX = (inputTexture->getWidth() + x - 1) / x;
    int groupY = (inputTexture->getHeight() + y - 1) / y;

    computeEncoder->Dispatch(groupX, groupY, 1);
}

RenderTexturePtr TestImageGray()
{
    ShaderAssetString shaderAssetString = LoadShaderAsset("TestImageGray");
    
    const char* computeShader = nullptr;
    if (getRenderDevice()->getRenderDeviceType() == RenderDeviceType::METAL)
    {
        computeShader = shaderAssetString.metalShader.computeShaderStr.c_str();
    }
    
    FILE* fp1 = fopen("/Users/zhouxuguang/work/TestImageGray.metal", "wb");
    fwrite(computeShader, 1, strlen(computeShader), fp1);
    fclose(fp1);
    
    // 创建纹理
    std::string strPath = getMediaDir() + "DamagedHelmet/glTF/Default_albedo.jpg";
    VImagePtr image = std::make_shared<VImage>();
    ImageDecoder::DecodeFile(strPath.c_str(), image.get());
    
    TextureDescriptor inputDes = ImageTextureUtil::getTextureDescriptor(*image);
    inputDes.format = kTexFormatRGBA32;
    
    Texture2DPtr inputTexture = getRenderDevice()->createTextureWithDescriptor(inputDes);
    inputTexture->setTextureData(image->GetPixels());
    
    //inputDes.usage = TextureUsageShaderRead | TextureUsageShaderWrite;
    inputDes.format = kTexFormatRGBA32;
    RenderTexturePtr outputTexture = getRenderDevice()->createRenderTexture(inputDes);
    
    ComputePipelinePtr computePipeline = getRenderDevice()->createComputePipeline(computeShader);
    
    CommandBufferPtr command = getRenderDevice()->createCommandBuffer();
    
    ComputeEncoderPtr computeEncoder = command->createComputeEncoder();
    computeEncoder->SetComputePipeline(computePipeline);
    computeEncoder->SetTexture(inputTexture, 0);
    computeEncoder->SetTexture(outputTexture, 0, 1);
    
    uint32_t x, y ,z;
    computePipeline->GetThreadGroupSizes(x, y, z);
    
    int groupX = (image->GetWidth() + x - 1) / x;
    int groupY = (image->GetHeight() + y - 1) / y;

    computeEncoder->Dispatch(groupX, groupY, 1);
    
    computeEncoder->EndEncode();
    command->waitUntilCompleted();
    
    return outputTexture;
}