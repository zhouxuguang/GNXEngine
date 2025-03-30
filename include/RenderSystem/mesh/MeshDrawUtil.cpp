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
    std::string pathSplit = std::string(1, PATHSPLIT);
    if (!envMap)
    {
        envMap = LoadEquirectangularMap(getMediaDir() + "IBL" + pathSplit + "piazza_bologni_1k.hdr");
    }
    
    if (!envMapIrradiance)
    {
        envMapIrradiance = LoadEquirectangularMap(getMediaDir() + "IBL" + pathSplit + "piazza_bologni_1k_irradiance.hdr");
    }
    
    if (!cubeSampler)
    {
        SamplerDescriptor sampleDes;
        sampleDes.filterMip = MIN_LINEAR_MIPMAP_LINEAR;
        sampleDes.maxLod = 8;
        cubeSampler = getRenderDevice()->createSamplerWithDescriptor(sampleDes);
    }
    
    if (!brdfMap) {
        VImage image;
        imagecodec::ImageDecoder::DecodeFile((getMediaDir() + "IBL" + pathSplit + "brdfLUT.ktx").c_str(), &image);
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
        
        renderEncoder->setVertexUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
        renderEncoder->setVertexUniformBuffer("cbPerObject", renderInfo.objectUBO);
        renderEncoder->setVertexUniformBuffer("LightInfo", renderInfo.lightUBO);
        
        renderEncoder->setFragmentUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
        renderEncoder->setFragmentUniformBuffer("LightInfo", renderInfo.lightUBO);
        
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
        renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
        
        MaterialPtr material = renderInfo.materials[n];
        assert(material);
        
        TextureSamplerPtr textureSampler = mesh.GetSampler();
        
        //这里感觉采样器和纹理封装在一个对象里面会比较方便
        renderEncoder->setFragmentTextureAndSampler("gDiffuseMap", material->GetTexture("diffuseTexture"), textureSampler);
        /*renderEncoder->setFragmentTextureAndSampler(material->GetTexture("normalTexture"), textureSampler, 1);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("roughnessTexture"), textureSampler, 2);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("emissiveTexture"), textureSampler, 3);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("ambientTexture"), textureSampler, 4);*/
        
        /*renderEncoder->setFragmentTextureCubeAndSampler(envMap, cubeSampler, 5);
        renderEncoder->setFragmentTextureCubeAndSampler(envMapIrradiance, cubeSampler, 6);
        renderEncoder->setFragmentTextureAndSampler(brdfMap, textureSampler, 7);*/
        
        const SubMeshInfo& subInfo = mesh.GetSubMeshInfo(n);
        
        renderEncoder->drawIndexedPrimitives(PrimitiveMode_TRIANGLES, (int)subInfo.indexCount, indexBuffer, subInfo.firstIndex);
        
    }
}

void MeshDrawUtil::DrawSkinnedMesh(const SkinnedMesh& mesh, const RenderInfo& renderInfo, bool isCPUSkin)
{
    std::string pathSplit = std::string(1, PATHSPLIT);
    if (!envMap)
    {
        envMap = LoadEquirectangularMap(getMediaDir() + "IBL" + pathSplit + "piazza_bologni_1k.hdr");
    }
    
    if (!envMapIrradiance)
    {
        envMapIrradiance = LoadEquirectangularMap(getMediaDir() + "IBL" + pathSplit + "piazza_bologni_1k_irradiance.hdr");
    }
    
    if (!cubeSampler)
    {
        SamplerDescriptor sampleDes;
        sampleDes.filterMip = MIN_LINEAR_MIPMAP_LINEAR;
        sampleDes.maxLod = 8;
        cubeSampler = getRenderDevice()->createSamplerWithDescriptor(sampleDes);
    }
    
    if (!brdfMap) {
        VImage image;
        imagecodec::ImageDecoder::DecodeFile((getMediaDir() + "IBL" + pathSplit + "brdfLUT.ktx").c_str(), &image);
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
        
		renderEncoder->setFragmentUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
		renderEncoder->setFragmentUniformBuffer("LightInfo", renderInfo.lightUBO);
        
        if (isCPUSkin)
        {
			renderEncoder->setVertexUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
			renderEncoder->setVertexUniformBuffer("cbPerObject", renderInfo.objectUBO);
			renderEncoder->setVertexUniformBuffer("LightInfo", renderInfo.lightUBO);
			
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
            renderEncoder->setVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
        }
        else
        {
			renderEncoder->setVertexUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
			renderEncoder->setVertexUniformBuffer("cbPerObject", renderInfo.objectUBO);
			renderEncoder->setVertexUniformBuffer("LightInfo", renderInfo.lightUBO);
            renderEncoder->setVertexUniformBuffer("cbSkinned", renderInfo.skinnedMatrixUBO);
            
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
        renderEncoder->setFragmentTextureAndSampler("", material->GetTexture("diffuseTexture"), textureSampler);
        /*renderEncoder->setFragmentTextureAndSampler(material->GetTexture("normalTexture"), textureSampler, 1);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("roughnessTexture"), textureSampler, 2);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("emissiveTexture"), textureSampler, 3);
        renderEncoder->setFragmentTextureAndSampler(material->GetTexture("ambientTexture"), textureSampler, 4);
        
        renderEncoder->setFragmentTextureCubeAndSampler(envMap, cubeSampler, 5);
        renderEncoder->setFragmentTextureCubeAndSampler(envMapIrradiance, cubeSampler, 6);
        renderEncoder->setFragmentTextureAndSampler(brdfMap, textureSampler, 7);*/
        
        const SubMeshInfo& subInfo = mesh.GetSubMeshInfo(n);
        
        renderEncoder->drawIndexedPrimitives(PrimitiveMode_TRIANGLES, (int)subInfo.indexCount, indexBuffer, subInfo.firstIndex);
        
    }
}

NS_RENDERSYSTEM_END
