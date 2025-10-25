//
//  MeshDrawUtil.cpp
//  rendersystem
//
//  Created by zhouxuguang on 2024/5/3.
//

#include "MeshDrawUtil.h"

//foe test
#include "ImageTextureUtil.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "RenderEngine.h"

RCTextureCubePtr envMap = nullptr;
RCTextureCubePtr envMapIrradiance = nullptr;
TextureSamplerPtr cubeSampler = nullptr;
RCTexture2DPtr brdfMap = nullptr;

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
        cubeSampler = GetRenderDevice()->CreateSamplerWithDescriptor(sampleDes);
    }
    
    if (!brdfMap) 
    {
        VImage image;
        imagecodec::ImageDecoder::DecodeFile((getMediaDir() + "IBL" + pathSplit + "brdfLUT.ktx").c_str(), &image);
        TextureDescriptor des = ImageTextureUtil::getTextureDescriptor(image);
        brdfMap = GetRenderDevice()->CreateTexture2D(des.format, TextureUsageShaderRead,
                                                     image.GetWidth(), image.GetHeight(), 1);
        Rect2D rect = Rect2D(0, 0, image.GetWidth(), image.GetHeight());
        brdfMap->ReplaceRegion(rect, 0, image.GetPixels(), image.GetBytesPerRow());
    }
    
    RenderEncoderPtr renderEncoder = renderInfo.renderEncoder;
    assert(renderEncoder);
    
    const ChannelInfo* channels = mesh.GetVertexData().GetChannels();
    VertexBufferPtr vertexBuffer = mesh.GetVertexBuffer();
    IndexBufferPtr indexBuffer = mesh.GetIndexBuffer();
    
    for (int n = 0; n < mesh.GetSubMeshCount(); n ++)
    {
        renderEncoder->SetGraphicsPipeline(renderInfo.materials[n]->GetPSO());
        
        renderEncoder->SetVertexUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
        renderEncoder->SetVertexUniformBuffer("cbPerObject", renderInfo.objectUBO);
        renderEncoder->SetVertexUniformBuffer("LightInfo", renderInfo.lightUBO);
        
        renderEncoder->SetFragmentUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
        renderEncoder->SetFragmentUniformBuffer("LightInfo", renderInfo.lightUBO);
        
        renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
        renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
        renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
        renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
        
        MaterialPtr material = renderInfo.materials[n];
        assert(material);
        
        TextureSamplerPtr textureSampler = mesh.GetSampler();
        
        //这里感觉采样器和纹理封装在一个对象里面会比较方便
        renderEncoder->SetFragmentTextureAndSampler("gDiffuseMap", material->GetTexture("diffuseTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gNormalMap", material->GetTexture("normalTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gMetalRoughMap", material->GetTexture("roughnessTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gEmissiveMap", material->GetTexture("emissiveTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gAmbientMap", material->GetTexture("ambientTexture"), textureSampler);
        
        renderEncoder->SetFragmentTextureAndSampler("texEnvMap", envMap, cubeSampler);
        renderEncoder->SetFragmentTextureAndSampler("texEnvMapIrradiance", envMapIrradiance, cubeSampler);
        renderEncoder->SetFragmentTextureAndSampler("texBRDF_LUT", brdfMap, textureSampler);
        
        const SubMeshInfo& subInfo = mesh.GetSubMeshInfo(n);
        
        renderEncoder->DrawIndexedPrimitives(PrimitiveMode_TRIANGLES, (int)subInfo.indexCount, indexBuffer, subInfo.firstIndex);
        
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
        cubeSampler = GetRenderDevice()->CreateSamplerWithDescriptor(sampleDes);
    }
    
    if (!brdfMap) 
    {
        VImage image;
        imagecodec::ImageDecoder::DecodeFile((getMediaDir() + "IBL" + pathSplit + "brdfLUT.ktx").c_str(), &image);
        TextureDescriptor des = ImageTextureUtil::getTextureDescriptor(image);
        brdfMap = GetRenderDevice()->CreateTexture2D(des.format, TextureUsageShaderRead,
                                                     image.GetWidth(), image.GetHeight(), 1);
        Rect2D rect = Rect2D(0, 0, image.GetWidth(), image.GetHeight());
        brdfMap->ReplaceRegion(rect, 0, image.GetPixels(), image.GetBytesPerRow());
    }
    
    RenderEncoderPtr renderEncoder = renderInfo.renderEncoder;
    assert(renderEncoder);
    
    const ChannelInfo* channels = mesh.GetVertexData().GetChannels();
    VertexBufferPtr vertexBuffer = mesh.GetVertexBuffer();
    IndexBufferPtr indexBuffer = mesh.GetIndexBuffer();
    
    for (int n = 0; n < mesh.GetSubMeshCount(); n ++)
    {
        renderEncoder->SetGraphicsPipeline(renderInfo.materials[n]->GetPSO());
        
		renderEncoder->SetFragmentUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
		renderEncoder->SetFragmentUniformBuffer("cbLighting", renderInfo.lightUBO);
        
        if (isCPUSkin)
        {
			renderEncoder->SetVertexUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
			renderEncoder->SetVertexUniformBuffer("cbPerObject", renderInfo.objectUBO);
			renderEncoder->SetVertexUniformBuffer("cbLighting", renderInfo.lightUBO);
			
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
        }
        else
        {
			renderEncoder->SetVertexUniformBuffer("cbPerCamera", renderInfo.cameraUBO);
			renderEncoder->SetVertexUniformBuffer("cbPerObject", renderInfo.objectUBO);
			renderEncoder->SetVertexUniformBuffer("cbLighting", renderInfo.lightUBO);
            renderEncoder->SetVertexUniformBuffer("cbSkinned", renderInfo.skinnedMatrixUBO);
            
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelPosition].offset, 0);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelNormal].offset, 1);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTangent].offset, 2);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelTexCoord0].offset, 3);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelBoneIndex].offset, 4);
            renderEncoder->SetVertexBuffer(vertexBuffer, channels[kShaderChannelWeight].offset, 5);
        }
        
        MaterialPtr material = renderInfo.materials[n];
        assert(material);
        
        TextureSamplerPtr textureSampler = mesh.GetSampler();
        
        //这里感觉采样器和纹理封装在一个对象里面会比较方便
        renderEncoder->SetFragmentTextureAndSampler("gDiffuseMap", material->GetTexture("diffuseTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gNormalMap", material->GetTexture("normalTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gMetalRoughMap", material->GetTexture("roughnessTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gEmissiveMap", material->GetTexture("emissiveTexture"), textureSampler);
        renderEncoder->SetFragmentTextureAndSampler("gAmbientMap", material->GetTexture("ambientTexture"), textureSampler);
        
        renderEncoder->SetFragmentTextureAndSampler("texEnvMap", envMap, cubeSampler);
        renderEncoder->SetFragmentTextureAndSampler("texEnvMapIrradiance", envMapIrradiance, cubeSampler);
        renderEncoder->SetFragmentTextureAndSampler("texBRDF_LUT", brdfMap, textureSampler);
        
        const SubMeshInfo& subInfo = mesh.GetSubMeshInfo(n);
        
        renderEncoder->DrawIndexedPrimitives(PrimitiveMode_TRIANGLES, (int)subInfo.indexCount, indexBuffer, subInfo.firstIndex);
        
    }
}

NS_RENDERSYSTEM_END
