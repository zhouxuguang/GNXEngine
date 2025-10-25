//
//  ShaderAsset.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/20.
//

#ifndef GNX_ENGINE_SHADER_ASSET_INCLUDE_H
#define GNX_ENGINE_SHADER_ASSET_INCLUDE_H

#include "RSDefine.h"
#include "ShaderAssetLoader.h"
#include "MathUtil/Vector3.h"
#include <unordered_map>
#include <string>

NS_RENDERSYSTEM_BEGIN

class ShaderAsset
{
public:
    ShaderAsset();
    
    ~ShaderAsset();
    
    bool LoadFromFile(const std::string& fileName);
    
    const ShaderCode& GetCompiledVertexShader() const;
    
    const ShaderCode& GetCompiledFragmentShader() const;
    
private:
    std::string mShaderStr;     //原始shader资产
    ShaderAssetString mCompiledShaderString;
};

typedef std::shared_ptr<ShaderAsset> ShaderAssetPtr;

NS_RENDERSYSTEM_END

#endif /* GNX_ENGINE_SHADER_ASSET_INCLUDE_H */
