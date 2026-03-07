//
//  Material.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#include "Material.h"
#include "ShaderAssetLoader.h"
#include "RenderEngine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"

NS_RENDERSYSTEM_BEGIN

Material::Material()
{
    //
}

Material::~Material()
{
    //
}

bool Material::HasProperty(const std::string& name)
{
    return true;
}

std::string Material::GetName() const
{
    return mName;
}

void Material::SetName(const std::string& name)
{
    mName = name;
}

// 获得默认的材质
Material *Material::GetDefault()
{
    return nullptr;
}

MaterialPtr Material::GetDefaultDiffuseMaterial()
{
    return Material::CreateMaterial("ModelShader");
}

MaterialPtr Material::CreateMaterial(const char *shaderStrPath)
{
    ShaderAssetString shaderAssetString = LoadShaderAsset(shaderStrPath);
    ShaderCodePtr vertexShader = shaderAssetString.vertexShader->shaderSource;
    ShaderCodePtr fragmentShader = shaderAssetString.fragmentShader->shaderSource;
    
    /*FILE* fp1 = fopen("/Users/zhouxuguang/work/pbr.vert", "wb");
    fwrite(vertexShader->data(), 1, vertexShader->size(), fp1);
    fclose(fp1);

    FILE* fp2 = fopen("/Users/zhouxuguang/work/pbr.frag", "wb");
    fwrite(fragmentShader->data(), 1, fragmentShader->size(), fp2);
    fclose(fp2);*/
    
    /*ShaderFunctionPtr vertShader = getRenderDevice()->createShaderFunction(*vertexShader, ShaderStage_Vertex);
    ShaderFunctionPtr fragShader = getRenderDevice()->createShaderFunction(*fragmentShader, ShaderStage_Fragment);*/

    GraphicsShaderPtr shader = GetRenderDevice()->CreateGraphicsShader(*vertexShader, *fragmentShader);
    
    GraphicsPipelineDesc graphicsPipelineDescriptor;
    VertextAttributesDesc vertextAttributesDescritptor;
    VertexBufferLayoutDesc vertexBufferLayoutDescriptor;
    
    graphicsPipelineDescriptor.vertexDescriptor = shaderAssetString.vertexDescriptor;
    
    graphicsPipelineDescriptor.depthStencilDescriptor.depthCompareFunction = DepthConfig::GetDefaultDepthCompareFunc();
    graphicsPipelineDescriptor.depthStencilDescriptor.depthWriteEnabled = true;
    
    GraphicsPipelinePtr pso = GetRenderDevice()->CreateGraphicsPipeline(graphicsPipelineDescriptor);
    pso->AttachGraphicsShader(shader);
    /*pso->attachVertexShader(vertShader);
    pso->attachFragmentShader(fragShader);*/
    
    MaterialPtr material = std::make_shared<Material>();
    material->SetPSO(pso);
    
    return material;
}

Material *Material::CreateMaterial(ShaderAssetPtr shader)
{
    return nullptr;
}

// 设置材质关联的shader
void Material::SetShader(ShaderAssetPtr shader)
{
    mShaderAsset = shader;
}

const ShaderAssetPtr Material::GetShader() const
{
    return mShaderAsset;
}

// 设置颜色
void Material::SetColor(const std::string& name, const mathutil::Vector4f &color)
{
    mVectorMapProps.emplace(name, color);
}

bool Material::GetColor(const std::string& name, mathutil::Vector4f &color)
{
    auto iter = mVectorMapProps.find(name);
    if (iter != mVectorMapProps.end())
    {
        color = iter->second;
        return true;
    }
    return false;
}

void Material::SetValue(const std::string& name, const float value)
{
    mFloatMapProps.emplace(name, value);
}

bool Material::GetValue(const std::string& name, float &value)
{
    auto iter = mFloatMapProps.find(name);
    if (iter != mFloatMapProps.end())
    {
        value = iter->second;
        return true;
    }
    return false;
}

void Material::SetTexture(const std::string& name, RCTexturePtr texture)
{
    mTextureMapProps.emplace(name, texture);
}

RCTexturePtr Material::GetTexture(const std::string& name)
{
    auto iter = mTextureMapProps.find(name);
    if (iter != mTextureMapProps.end())
    {
        return iter->second;
    }
    return nullptr;
}

// G-Buffer相关方法实现

void Material::SetMaterialType(MaterialType type)
{
    mMaterialType = type;
}

Material::MaterialType Material::GetMaterialType() const
{
    return mMaterialType;
}

void Material::SetGBufferShader(ShaderAssetPtr shader)
{
    mGBufferShaderAsset = shader;
}

const ShaderAssetPtr Material::GetGBufferShader() const
{
    return mGBufferShaderAsset;
}

void Material::SetGBufferPSO(GraphicsPipelinePtr pso)
{
    mGBufferPSO = pso;
}

GraphicsPipelinePtr Material::GetGBufferPSO() const
{
    return mGBufferPSO;
}

const char* Material::GetGBufferShaderPath(MaterialType type)
{
    switch (type)
    {
        case MaterialType::PBR:
            return "GBufferPBR";
        case MaterialType::Diffuse:
            return "GBufferDiffuse";
        case MaterialType::SkinPBR:
            return "GBufferSkinPBR";  // 需要另外实现
        case MaterialType::Unlit:
            return "GBufferUnlit";    // 需要另外实现
        default:
            return "GBufferPBR";
    }
}

NS_RENDERSYSTEM_END
