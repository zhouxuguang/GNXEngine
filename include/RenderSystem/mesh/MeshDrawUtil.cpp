//
//  MeshDrawUtil.cpp
//  rendersystem
//
//  Created by zhouxuguang on 2024/5/3.
//

#include "MeshDrawUtil.h"

//foe test
#include "ImageTextureUtil.h"
#include "RenderEngine.h"

TextureCubePtr envMap = nullptr;
TextureCubePtr envMapIrradiance = nullptr;
TextureSamplerPtr cubeSampler = nullptr;
Texture2DPtr brdfMap = nullptr;

NS_RENDERSYSTEM_BEGIN


void MeshDrawUtil::DrawMesh(const Mesh& mesh, const RenderInfo& renderInfo)
{
    if (!envMap)
    {
        envMap = LoadEquirectangularMap(getMediaDir() + "IBL/piazza_bologni_1k.hdr");
    }
    
    if (!envMapIrradiance)
    {
        envMapIrradiance = LoadEquirectangularMap(getMediaDir() + "IBL/piazza_bologni_1k_irradiance.hdr");
    }
    
    if (!cubeSampler)
    {
        SamplerDescriptor sampleDes;
        sampleDes.filterMin = MIN_LINEAR_MIPMAP_LINEAR;
        sampleDes.maxLod = 8;
        cubeSampler = getRenderDevice()->createSamplerWithDescriptor(sampleDes);
    }
    
    if (!brdfMap) {
        VImage image;
        imagecodec::ImageDecoder::DecodeFile((getMediaDir() + "IBL/brdfLUT.ktx").c_str(), &image);
        TextureDescriptor des = ImageTextureUtil::getTextureDescriptor(image);
        brdfMap = getRenderDevice()->createTextureWithDescriptor(des);
        brdfMap->setTextureData(image.GetPixels());
    }
    
    RenderEncoderPtr renderEncoder = renderInfo.renderEncoder;
    assert(renderEncoder);
    
    const ChannelInfo* channels = mesh.GetVertexData().GetChannels();
    VertexBufferPtr vertexBuffer = mesh.GetVertexBuffer();
    IndexBufferPtr indexBuffer = mesh.GetIndexBuffer();
    
    for (int n = 0; n < mesh.GetSubMeshCount(); n ++)
    {
        renderEncoder->setGraphicsPipeline(renderInfo.materials[n]->GetPSO());
        
        renderEncoder->setVertexUniformBuffer(renderInfo.cameraUBO, 4);
        renderEncoder->setVertexUniformBuffer(renderInfo.objectUBO, 5);
        renderEncoder->setVertexUniformBuffer(renderInfo.lightUBO, 6);
        
        renderEncoder->setFragmentUniformBuffer(renderInfo.cameraUBO, 0);
        renderEncoder->setFragmentUniformBuffer(renderInfo.lightUBO, 2);
        
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
        
        MaterialPtr material = renderInfo.materials[n];
        assert(material);
        
        TextureSamplerPtr textureSampler = mesh.GetSampler();
        
        //这里感觉采样器和纹理封装在一个对象里面会比较方便
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("diffuseTexture"), textureSampler, 0);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("normalTexture"), textureSampler, 1);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("roughnessTexture"), textureSampler, 2);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("emissiveTexture"), textureSampler, 3);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("ambientTexture"), textureSampler, 4);
        
        renderEncoder->setFragmentTextureCubeAndSampler(envMap, cubeSampler, 5);
        renderEncoder->setFragmentTextureCubeAndSampler(envMapIrradiance, cubeSampler, 6);
        renderEncoder->setFragmentTextureAndSampler(brdfMap, textureSampler, 7);
        
        const SubMeshInfo& subInfo = mesh.GetSubMeshInfo(n);
        
        renderEncoder->drawIndexedPrimitives(PrimitiveMode_TRIANGLES, (int)subInfo.indexCount, indexBuffer, subInfo.firstIndex);
        
    }
}

void MeshDrawUtil::DrawSkinnedMesh(const SkinnedMesh& mesh, const RenderInfo& renderInfo, bool isCPUSkin)
{
    if (!envMap)
    {
        envMap = LoadEquirectangularMap(getMediaDir() + "IBL/piazza_bologni_1k.hdr");
    }
    
    if (!envMapIrradiance)
    {
        envMapIrradiance = LoadEquirectangularMap(getMediaDir() + "IBL/piazza_bologni_1k_irradiance.hdr");
    }
    
    if (!cubeSampler)
    {
        SamplerDescriptor sampleDes;
        sampleDes.filterMin = MIN_LINEAR_MIPMAP_LINEAR;
        sampleDes.maxLod = 8;
        cubeSampler = getRenderDevice()->createSamplerWithDescriptor(sampleDes);
    }
    
    if (!brdfMap) {
        VImage image;
        imagecodec::ImageDecoder::DecodeFile((getMediaDir() + "IBL/brdfLUT.ktx").c_str(), &image);
        TextureDescriptor des = ImageTextureUtil::getTextureDescriptor(image);
        brdfMap = getRenderDevice()->createTextureWithDescriptor(des);
        brdfMap->setTextureData(image.GetPixels());
    }
    
    RenderEncoderPtr renderEncoder = renderInfo.renderEncoder;
    assert(renderEncoder);
    
    const ChannelInfo* channels = mesh.GetVertexData().GetChannels();
    VertexBufferPtr vertexBuffer = mesh.GetVertexBuffer();
    IndexBufferPtr indexBuffer = mesh.GetIndexBuffer();
    
    for (int n = 0; n < mesh.GetSubMeshCount(); n ++)
    {
        renderEncoder->setGraphicsPipeline(renderInfo.materials[n]->GetPSO());
        
        renderEncoder->setFragmentUniformBuffer(renderInfo.cameraUBO, 0);
        renderEncoder->setFragmentUniformBuffer(renderInfo.lightUBO, 2);
        
        if (isCPUSkin)
        {
            renderEncoder->setVertexUniformBuffer(renderInfo.cameraUBO, 0);
            renderEncoder->setVertexUniformBuffer(renderInfo.objectUBO, 1);
            renderEncoder->setVertexUniformBuffer(renderInfo.lightUBO, 2);
            
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
        }
        else
        {
            renderEncoder->setVertexUniformBuffer(renderInfo.cameraUBO, 0);
            renderEncoder->setVertexUniformBuffer(renderInfo.objectUBO, 1);
            renderEncoder->setVertexUniformBuffer(renderInfo.lightUBO, 2);
            renderEncoder->setVertexUniformBuffer(renderInfo.skinnedMatrixUBO, 3);
            
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelBoneIndex].offset, 4);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelWeight].offset, 5);
        }
        
        MaterialPtr material = renderInfo.materials[n];
        assert(material);
        
        TextureSamplerPtr textureSampler = mesh.GetSampler();
        
        //这里感觉采样器和纹理封装在一个对象里面会比较方便
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("diffuseTexture"), textureSampler, 0);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("normalTexture"), textureSampler, 1);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("roughnessTexture"), textureSampler, 2);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("emissiveTexture"), textureSampler, 3);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("ambientTexture"), textureSampler, 4);
        
        renderEncoder->setFragmentTextureCubeAndSampler(envMap, cubeSampler, 5);
        renderEncoder->setFragmentTextureCubeAndSampler(envMapIrradiance, cubeSampler, 6);
        renderEncoder->setFragmentTextureAndSampler(brdfMap, textureSampler, 7);
        
        const SubMeshInfo& subInfo = mesh.GetSubMeshInfo(n);
        
        renderEncoder->drawIndexedPrimitives(PrimitiveMode_TRIANGLES, (int)subInfo.indexCount, indexBuffer, subInfo.firstIndex);
        
    }
}

NS_RENDERSYSTEM_END
