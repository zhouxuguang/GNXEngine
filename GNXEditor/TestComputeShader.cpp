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
    ComputeBufferPtr buffer1 = GetRenderDevice()->CreateComputeBuffer(data_a, count * 4, StorageModePrivate);
    
    float *data_b = new float[count];
    for (int i = 0; i < count; i ++)
    {
        data_b[i] = (i + 1) * 2;
    }
    ComputeBufferPtr buffer2 = GetRenderDevice()->CreateComputeBuffer(data_b, count * 4, StorageModePrivate);
    
    ComputeBufferPtr buffer3 = GetRenderDevice()->CreateComputeBuffer(data_b, count * 4, StorageModeShared);
    
    ComputePipelinePtr computePipeline = GetRenderDevice()->CreateComputePipeline(*computeShader);
    
    CommandBufferPtr command = GetRenderDevice()->CreateCommandBuffer();
    
    ComputeEncoderPtr computeEncoder = command->CreateComputeEncoder();
    computeEncoder->SetComputePipeline(computePipeline);
    computeEncoder->SetBuffer(buffer1, 0);
    computeEncoder->SetBuffer(buffer2, 1);
    computeEncoder->SetBuffer(buffer3, 2);
    
    uint32_t x, y ,z;
    computePipeline->GetThreadGroupSizes(x, y, z);
    
    int groupX = (count + x - 1) / x;
    
    computeEncoder->Dispatch(groupX, 1, 1);
    
    computeEncoder->EndEncode();
    command->WaitUntilCompleted();
    
    //检查结果
    float* a = data_a;
    float* b = data_b;
    float* result = (float*)buffer3->MapBufferData();

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
    buffer3->UnmapBufferData(result);
    
    delete [] data_a;
    delete [] data_b;
}

#endif

ComputePipelinePtr initTestimageGray()
{
#if 1
    ShaderAssetString shaderAssetString = LoadShaderAsset("TestImageGray");
    
    ShaderCodePtr computeShader = shaderAssetString.computeShader->shaderSource;
    
    FILE* fp1 = fopen("/Users/zhouxuguang/work/TestImageGray.metal", "wb");
    fwrite(computeShader->data(), 1, computeShader->size(), fp1);
    fclose(fp1);
    
    ComputePipelinePtr computePipeline = GetRenderDevice()->CreateComputePipeline(*computeShader);
    
    return computePipeline;
#elif
    return nullptr;
#endif
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
