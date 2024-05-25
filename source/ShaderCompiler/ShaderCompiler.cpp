//
//  ShaderCompiler.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2021/5/9.
//

#include "ShaderCompiler.h"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_msl.hpp"
#include "DXCompilerUtil.h"
#include "ReflectionInfo.h"

NAMESPACE_SHADERCOMPILER_BEGIN

bool startsWith(const std::string& str, const std::string& prefix)
{
    return (str.rfind(prefix, 0) == 0);
}

//glslang使用说明
//https://stackoverflow.com/questions/38234986/how-to-use-glslang

std::string compileToESSL30(const std::vector<uint32_t>& spirvCode, ShaderStage shaderStage)
{
    spirv_cross::CompilerGLSL glsl(std::move(spirvCode));

    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    // Get all sampled images in the shader.
    for (auto &resource : resources.sampled_images)
    {
        unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);

        // Modify the decoration to prepare it for GLSL.
        glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

        // Some arbitrary remapping if we want.
        glsl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
    }
    
    // 对顶点输出和着色输入进行改装
    if (ShaderStage_Vertex == shaderStage)
    {
        for (auto &resource : resources.stage_outputs)
        {
            std::string outName = resource.name;
            if (startsWith(outName, "out."))
            {
                glsl.set_name(resource.id, outName.substr(4));
            }
        }
    }
    
    if (ShaderStage_Fragment == shaderStage)
    {
        for (auto &resource : resources.stage_inputs)
        {
            std::string inName = resource.name;
            if (startsWith(inName, "in."))
            {
                glsl.set_name(resource.id, inName.substr(3));
            }
        }
    }
    
    //处理纹理和采样器
    // Builds a mapping for all combinations of images and samplers.
    glsl.build_combined_image_samplers();

    // Give the remapped combined samplers new names.
    // Here you can also set up decorations if you want (binding = #N).
    for (auto &remap : glsl.get_combined_image_samplers())
    {
        glsl.set_name(remap.combined_id, spirv_cross::join("SPIRV_Cross_Combined", glsl.get_name(remap.image_id),
                                              glsl.get_name(remap.sampler_id)));
    }
    
    // Set some options.
    spirv_cross::CompilerGLSL::Options options;
    options.version = 300;
    options.es = true;
    glsl.set_common_options(options);

    // Compile to GLSL, ready to give to GL driver.
    return glsl.compile();
}

CompiledShaderInfo compileToMSL(const std::vector<uint32_t>& spirvCode, ShaderStage shaderStage)
{
    spirv_cross::CompilerMSL msl(std::move(spirvCode));

    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = msl.get_shader_resources();

    // Get all sampled images in the shader.
    for (auto &resource : resources.sampled_images)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);

        // Modify the decoration to prepare it for GLSL.
        msl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

        // Some arbitrary remapping if we want.
        msl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
    }
    
    //修改顶点着色器中uniform——buffer的索引
    if (shaderStage == ShaderStage_Vertex)
    {
        for (auto &resource : resources.uniform_buffers)
        {
            uint32_t index = msl.get_decoration(resource.id, spv::DecorationBinding);
            msl.set_decoration(resource.id, spv::DecorationBinding, index + (uint32_t)resources.stage_inputs.size());
        }
        
        int index = 0;
        for (auto &resource : resources.stage_inputs)
        {
            uint32_t index1 = msl.get_decoration(resource.id, spv::DecorationLocation);
            
            const spirv_cross::SPIRType &type = msl.get_type(resource.type_id);
            msl.set_decoration(resource.id, spv::DecorationLocation, index);
            index ++;
        }
    }
    
    spirv_cross::CompilerMSL::Options options;
#if defined(TARGET_OS_IOS)
    options.platform = spirv_cross::CompilerMSL::Options::iOS;
#else
    options.platform = spirv_cross::CompilerMSL::Options::macOS;
#endif
    
    options.enable_decoration_binding = true;
    msl.set_msl_options(options);
//    spirv_cross::MSLResourceBinding resourceBinding;
//    msl.add_msl_resource_binding(resourceBinding);

    // Compile to msl, ready to give to metal driver.
    std::string shaderSource = msl.compile();
    
    CompiledShaderInfo shaderInfo;
    if (shaderStage == ShaderStage_Vertex)
    {
        shaderInfo.vertexDescriptor = GetMetalReflectionInfo(msl, resources);
        shaderInfo.vertexUniformBufferLayout = GetMetalUniformReflectionInfo(msl, resources);
    }
    else
    {
        shaderInfo.fragmentUniformBufferLayout = GetMetalUniformReflectionInfo(msl, resources);
    }
    
    shaderInfo.shaderSource = std::move(shaderSource);
    
    return shaderInfo;
}

//HLSL shader脚本字符串转换
ShaderCodePtr compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage)
{
    return DXCompilerUtil::GetInstance()->compileHLSLToSPIRV(shaderFile, shaderStage);
}

NAMESPACE_SHADERCOMPILER_END