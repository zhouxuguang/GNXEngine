//
//  MTLShaderFunction.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/8/27.
//

#ifndef GNX_ENGINE_SHADER_FUNCTION_INCLUDE_H
#define GNX_ENGINE_SHADER_FUNCTION_INCLUDE_H

#include "MTLRenderDefine.h"
#include "ShaderFunction.h"
#include <unordered_map>

NAMESPACE_RENDERCORE_BEGIN

// 需要注意 : std::enable_shared_from_this 要public继承
class MTLShaderFunction : public ShaderFunction , public std::enable_shared_from_this<MTLShaderFunction>
{
public:
    MTLShaderFunction(id<MTLDevice> device);
    
    ~MTLShaderFunction();
    
    virtual ShaderFunctionPtr InitWithShaderSource(const ShaderCode& shaderSource, ShaderStage shaderStage);
    
    virtual ShaderStage GetShaderStage() const;
    
    id<MTLFunction> GetShaderFunction() const;
    
private:
    ShaderStage mShaderStage;
    id<MTLLibrary> mLibrary;
    id<MTLDevice> mDevice;
    id<MTLFunction> mFunction;
};

using MTLShaderFunctionPtr = std::shared_ptr<MTLShaderFunction>;

#pragma mark region MTLGraphicsShader

const NSUInteger InvalidBindingIndex = -1;

class MTLGraphicsShader : public GraphicsShader
{
public:
    MTLGraphicsShader(id<MTLDevice> device, const ShaderCode& vertexShader, const ShaderCode& fragmentShader);
    
    // Mesh Shader 构造函数 (Task + Mesh + Fragment)
    MTLGraphicsShader(id<MTLDevice> device, const ShaderCode& taskShader, const ShaderCode& meshShader, const ShaderCode& fragmentShader);
    
    void GenerateRefectionInfo(MTLRenderPipelineReflection* reflectionObj);
    
    // Mesh Pipeline 反射信息
    void GenerateMeshRefectionInfo(MTLRenderPipelineReflection* reflectionObj);
    
    id<MTLFunction> GetVertexFunction() const
    {
        return mVertexFunction;
    }
    
    id<MTLFunction> GetFragmentFunction() const
    {
        return mFragmentFunction;
    }
    
    id<MTLFunction> GetTaskFunction() const
    {
        return mTaskFunction;
    }
    
    id<MTLFunction> GetMeshFunction() const
    {
        return mMeshFunction;
    }
    
    bool IsMeshShader() const
    {
        return mMeshFunction != nil;
    }
    
    virtual std::string GetName() const
    {
        return "";
    }
    
    NSUInteger GetVertexResourceBindIndex(const std::string& resourceName) const
    {
        auto iter = mVertexBindings.find(resourceName);
        if (iter != mVertexBindings.end())
        {
            return iter->second;
        }
        
        return InvalidBindingIndex;
    }
    
    NSUInteger GetFragmentResourceBindIndex(const std::string& resourceName) const
    {
        auto iter = mFragmentBindings.find(resourceName);
        if (iter != mFragmentBindings.end())
        {
            return iter->second;
        }

        return InvalidBindingIndex;
    }
    
    NSUInteger GetMeshResourceBindIndex(const std::string& resourceName) const
    {
        auto iter = mMeshBindings.find(resourceName);
        if (iter != mMeshBindings.end())
        {
            return iter->second;
        }
        return InvalidBindingIndex;
    }
    
    NSUInteger GetTaskResourceBindIndex(const std::string& resourceName) const
    {
        auto iter = mTaskBindings.find(resourceName);
        if (iter != mTaskBindings.end())
        {
            return iter->second;
        }
        return InvalidBindingIndex;
    }
private:
    id<MTLFunction> mVertexFunction = nil;
    id<MTLFunction> mFragmentFunction = nil;
    id<MTLFunction> mTaskFunction = nil;
    id<MTLFunction> mMeshFunction = nil;
    std::unordered_map<std::string, NSUInteger> mVertexBindings;
    std::unordered_map<std::string, NSUInteger> mFragmentBindings;
    std::unordered_map<std::string, NSUInteger> mMeshBindings;
    std::unordered_map<std::string, NSUInteger> mTaskBindings;
};

using MTLGraphicsShaderPtr = std::shared_ptr<MTLGraphicsShader>;

NAMESPACE_RENDERCORE_END

#endif /* GNX_ENGINE_SHADER_FUNCTION_INCLUDE_H */
