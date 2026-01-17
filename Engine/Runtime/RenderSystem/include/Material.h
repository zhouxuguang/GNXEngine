//
//  Material.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef GNXENGINE_MATERIAL_SDDFJGJ_INCLUDE
#define GNXENGINE_MATERIAL_SDDFJGJ_INCLUDE

#include "RSDefine.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include "Runtime/RenderCore/include/RCTexture.h"
#include "Runtime/RenderCore/include/GraphicsPipeline.h"
#include "ShaderAsset.h"
#include <unordered_map>
#include <string>

NS_RENDERSYSTEM_BEGIN

class RENDERSYSTEM_API Material
{
public:
    // 材质类型枚举
    enum class MaterialType
    {
        PBR,           // PBR材质
        Diffuse,       // 简单漫反射
        SkinPBR,       // 蒙皮PBR材质
        Unlit          // 无光照材质
    };
    
    Material();
    
    ~Material();
    
    bool HasProperty(const std::string& name);
    
    std::string GetName() const;
    void SetName(const std::string& name);
    
    // 获得默认的材质
    static Material *GetDefault();
    static std::shared_ptr<Material> GetDefaultDiffuseMaterial();
    
    static std::shared_ptr<Material> CreateMaterial(const char *shaderStrPath);
    static Material *CreateMaterial(ShaderAssetPtr shader);
    
    // 设置材质关联的shader
    void SetShader(ShaderAssetPtr shader);
    const ShaderAssetPtr GetShader() const;
    
    // G-Buffer相关方法
    void SetMaterialType(MaterialType type);
    MaterialType GetMaterialType() const;
    
    // 设置G-Buffer shader
    void SetGBufferShader(ShaderAssetPtr shader);
    const ShaderAssetPtr GetGBufferShader() const;
    
    // 设置G-Buffer PSO
    void SetGBufferPSO(GraphicsPipelinePtr pso);
    GraphicsPipelinePtr GetGBufferPSO() const;
    
    // 获取材质对应的G-Buffer shader路径（用于自动加载）
    static const char* GetGBufferShaderPath(MaterialType type);
    
    // 设置颜色
    void SetColor(const std::string& name, const mathutil::Vector4f &color);
    bool GetColor(const std::string& name, mathutil::Vector4f &color);
    
    void SetValue(const std::string& name, const float value);
    bool GetValue(const std::string& name, float &value);
    
    void SetTexture(const std::string& name, RCTexturePtr texture);
    RCTexturePtr GetTexture(const std::string& name);
    
    void SetPSO(GraphicsPipelinePtr pso)
    {
        mPSO = pso;
    }
    
    GraphicsPipelinePtr GetPSO() const
    {
        return mPSO;
    }
    
private:
    typedef std::unordered_map<std::string, RCTexturePtr> TextureMap;
    TextureMap mTextureMapProps;   //纹理信息
    
    typedef std::unordered_map<std::string, float> FloatMap;
    FloatMap mFloatMapProps;     //浮点信息
    
    typedef std::unordered_map<std::string, mathutil::Vector4f> Vector4fMap;
    Vector4fMap mVectorMapProps;  //vector4
    
    typedef std::unordered_map<std::string, int> IntMap;
    IntMap mIntMapProps;      //字符串信息
    
    ShaderAssetPtr mShaderAsset = nullptr;
    
    std::string mName;
    
    GraphicsPipelinePtr mPSO = nullptr;
    
    // G-Buffer相关
    MaterialType mMaterialType = MaterialType::PBR;
    ShaderAssetPtr mGBufferShaderAsset = nullptr;
    GraphicsPipelinePtr mGBufferPSO = nullptr;
};

typedef std::shared_ptr<Material> MaterialPtr;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_MATERIAL_SDDFJGJ_INCLUDE */
