//
//  TestComputeShader.cpp
//  GNXEditor
//
//  Created by zhouxuguang on 2024/5/12.
//

#include "TestComputeShader.hpp"
#include "Runtime/RenderSystem/include/ShaderAssetLoader.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/ImageCodec/include/ImageDecoder.h"
#include "Runtime/RenderSystem/include/ImageTextureUtil.h"
#include <assert.h>

using namespace imagecodec;

using namespace RenderSystem;

#if 1
void TestADD()
{
    ShaderAssetString shaderAssetString = LoadShaderAsset("TestADD");
    
    ShaderCodePtr computeShader = shaderAssetString.computeShader->shaderSource;
    
    FILE* fp1 = fopen("/Users/zhouxuguang/work/TestADD.metal", "wb");
    fwrite(computeShader->data(), 1, computeShader->size(), fp1);
    fclose(fp1);
    
    int count = 1 << 26;
    float *data_a = new float[count];
    for (int i = 0; i < count; i ++)
    {
        data_a[i] = i + 1;
    }
    RCBufferDesc desc1(count * 4, RCBufferUsage::StorageBuffer, StorageModePrivate);
    RCBufferPtr buffer1 = GetRenderDevice()->CreateBuffer(desc1, data_a);
    
    float *data_b = new float[count];
    for (int i = 0; i < count; i ++)
    {
        data_b[i] = (i + 1) * 2;
    }
    RCBufferDesc desc2(count * 4, RCBufferUsage::StorageBuffer, StorageModePrivate);
    RCBufferPtr buffer2 = GetRenderDevice()->CreateBuffer(desc2, data_b);
    
    RCBufferDesc desc3(count * 4, RCBufferUsage::StorageBuffer, StorageModeShared);
    RCBufferPtr buffer3 = GetRenderDevice()->CreateBuffer(desc3, data_b);
    
    ComputePipelinePtr computePipeline = GetRenderDevice()->CreateComputePipeline(*computeShader);
    
    // 从Compute队列创建命令缓冲区
    CommandQueuePtr computeQueue = GetRenderDevice()->GetCommandQueue(QueueType::Compute, 0);
    CommandBufferPtr command = computeQueue->CreateCommandBuffer();
    
    ComputeEncoderPtr computeEncoder = command->CreateComputeEncoder();
    computeEncoder->SetComputePipeline(computePipeline);
    computeEncoder->SetStorageBuffer(buffer1, 0);
    computeEncoder->SetStorageBuffer(buffer2, 1);
    computeEncoder->SetStorageBuffer(buffer3, 2);
    
    uint32_t x, y ,z;
    computePipeline->GetThreadGroupSizes(x, y, z);
    
    int groupX = (count + x - 1) / x;
    
    computeEncoder->Dispatch(groupX, 1, 1);
    
    computeEncoder->EndEncode();
    command->WaitUntilCompleted();
    
    //检查结果
    float* a = data_a;
    float* b = data_b;
    float* result = (float*)buffer3->Map();

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
    buffer3->Unmap();
    
    delete [] data_a;
    delete [] data_b;
}

#endif

ComputePipelinePtr initTestimageGray()
{
    ShaderAssetString shaderAssetString = LoadShaderAsset("TestImageGray");
    
    ShaderCodePtr computeShader = shaderAssetString.computeShader->shaderSource;
    
    ComputePipelinePtr computePipeline = GetRenderDevice()->CreateComputePipeline(*computeShader);
    
    return computePipeline;
}

void testImageGrayDraw(ComputeEncoderPtr computeEncoder, ComputePipelinePtr computePipeline,
                       RCTexturePtr inputTexture, RCTexturePtr outputTexture)
{
    computeEncoder->SetComputePipeline(computePipeline);
    computeEncoder->SetTexture(inputTexture, 0, 0);
    computeEncoder->SetOutTexture(outputTexture, 0, 1);
    
    uint32_t x, y ,z;
    computePipeline->GetThreadGroupSizes(x, y, z);
    
    uint32_t groupX = (inputTexture->GetWidth() + x - 1) / x;
    uint32_t groupY = (inputTexture->GetHeight() + y - 1) / y;

    computeEncoder->Dispatch(groupX, groupY, 1);
}
