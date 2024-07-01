//
//  Material.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef GNXENGINE_MATERIAL_SDDFJGJ_INCLUDE
#define GNXENGINE_MATERIAL_SDDFJGJ_INCLUDE

#include "RSDefine.h"
#include "MathUtil/Vector3.h"
#include "MathUtil/Vector4.h"
#include "RenderCore/Texture2D.h"
#include "RenderCore/GraphicsPipeline.h"
#include "ShaderAsset.h"
#include <unordered_map>
#include <string>

NS_RENDERSYSTEM_BEGIN

class Material
{
public:
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
    
    // 设置颜色
    void SetColor(const std::string& name, const mathutil::Vector4f &color);
    bool GetColor(const std::string& name, mathutil::Vector4f &color);
    
    void SetValue(const std::string& name, const float value);
    bool GetValue(const std::string& name, float &value);
    
    void SetTexture(const std::string& name, Texture2DPtr texture);
    Texture2DPtr GetTexture(const std::string& name);
    
    void SetPSO(GraphicsPipelinePtr pso)
    {
        mPSO = pso;
    }
    
    GraphicsPipelinePtr GetPSO() const
    {
        return mPSO;
    }
    
private:
    typedef std::unordered_map<std::string, Texture2DPtr> TextureMap;
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
};

typedef std::shared_ptr<Material> MaterialPtr;

NS_RENDERSYSTEM_END

#endif /* GNXENGINE_MATERIAL_SDDFJGJ_INCLUDE */
