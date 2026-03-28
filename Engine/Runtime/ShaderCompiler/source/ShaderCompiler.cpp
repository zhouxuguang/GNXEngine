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

// ShaderCompilerConfig 静态成员定义
// 注意：UseReverseZ 的值由上层 RenderSystem 的 BuildSetting 在初始化时同步
bool ShaderCompilerConfig::UseReverseZ = true;

bool startsWith(const std::string& str, const std::string& prefix)
{
    return (str.rfind(prefix, 0) == 0);
}

//glslang使用说明
//https://stackoverflow.com/questions/38234986/how-to-use-glslang

ShaderCode compileToESSL30(ShaderCodePtr spirvCode, ShaderStage shaderStage)
{
    spirv_cross::CompilerGLSL glsl((const uint32_t *)spirvCode->data(), spirvCode->size() / 4);

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
    std::string shaderStr = glsl.compile();
    ShaderCode shaderCode;
    shaderCode.resize(shaderStr.size());
    memcpy(shaderCode.data(), shaderStr.data(), shaderStr.size());
    
    return shaderCode;
}

CompiledShaderInfoPtr compileToMSL(ShaderCodePtr spirvCode, ShaderStage shaderStage)
{
    spirv_cross::CompilerMSL msl((const uint32_t*)spirvCode->data(), spirvCode->size() / 4);

    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = msl.get_shader_resources();

    spv::ExecutionModel model = (shaderStage == ShaderStage_Vertex)
        ? spv::ExecutionModelVertex : spv::ExecutionModelFragment;

    // 紧凑 sampled images 的 texture 和 sampler binding 到 0~N 连续索引
    // Metal 要求 sampler 索引必须在 0~15 范围内
    uint32_t nextTexBinding = 0;
    uint32_t nextSamBinding = 0;
    for (auto &resource : resources.sampled_images)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Image %s at set = %u, binding = %u -> MSL texture=%u, sampler=%u\n",
               resource.name.c_str(), set, binding, nextTexBinding, nextSamBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_texture = nextTexBinding;
        resBinding.msl_sampler = nextSamBinding;
        msl.add_msl_resource_binding(resBinding);

        nextTexBinding++;
        nextSamBinding++;
    }

    // 紧凑 separate samplers 的 binding（如果有独立的 sampler 变量）
    for (auto &resource : resources.separate_samplers)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Separate sampler %s at set = %u, binding = %u -> MSL sampler=%u\n",
               resource.name.c_str(), set, binding, nextSamBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_sampler = nextSamBinding;
        msl.add_msl_resource_binding(resBinding);

        nextSamBinding++;
    }

    // 紧凑 separate images 的 binding（如果有独立的 texture 变量）
    for (auto &resource : resources.separate_images)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("Separate image %s at set = %u, binding = %u -> MSL texture=%u\n",
               resource.name.c_str(), set, binding, nextTexBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_texture = nextTexBinding;
        msl.add_msl_resource_binding(resBinding);

        nextTexBinding++;
    }

    // 计算顶点属性占用的最大 location，用于调整 uniform buffer 起始索引
    uint32_t maxLocation = 0;
    int attrCount = 0;
    if (shaderStage == ShaderStage_Vertex)
    {
        for (auto &resource : resources.stage_inputs)
        {
            uint32_t loc = msl.get_decoration(resource.id, spv::DecorationLocation);
            attrCount++;
            if (loc > maxLocation)
            {
                maxLocation = loc;
            }
        }
        if (attrCount > 0)
        {
            maxLocation += 1;
        }
    }

    // 紧凑 uniform buffer binding
    uint32_t nextBufBinding = 0;
    if (shaderStage == ShaderStage_Vertex)
    {
        // 顶点着色器：buffer 索引从顶点属性之后开始
        nextBufBinding = maxLocation;
    }
    for (auto &resource : resources.uniform_buffers)
    {
        unsigned set = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
        printf("UBO %s at set = %u, binding = %u -> MSL buffer=%u\n",
               resource.name.c_str(), set, binding, nextBufBinding);

        spirv_cross::MSLResourceBinding resBinding;
        resBinding.stage = model;
        resBinding.desc_set = set;
        resBinding.binding = binding;
        resBinding.msl_buffer = nextBufBinding;
        msl.add_msl_resource_binding(resBinding);

        nextBufBinding++;
    }

    spirv_cross::CompilerMSL::Options options;
#if OS_IOS
    options.platform = spirv_cross::CompilerMSL::Options::iOS;
#else
    options.platform = spirv_cross::CompilerMSL::Options::macOS;
#endif

    // 关键：使用 MSLResourceBinding 代替 decoration binding
    options.enable_decoration_binding = false;
    msl.set_msl_options(options);

    // Compile to msl, ready to give to metal driver.
    std::string shaderSource = msl.compile();
    
    CompiledShaderInfoPtr shaderInfo = std::make_shared<CompiledShaderInfo>();
    if (shaderStage == ShaderStage_Vertex)
    {
        shaderInfo->vertexDescriptor = GetMetalReflectionInfo(msl, resources);
        //shaderInfo.vertexUniformBufferLayout = GetMetalUniformReflectionInfo(msl, resources);
    }
    else
    {
        //shaderInfo.fragmentUniformBufferLayout = GetMetalUniformReflectionInfo(msl, resources);
    }
    
    shaderInfo->shaderSource = std::make_shared<ShaderCode>();
    shaderInfo->shaderSource->resize(shaderSource.size());
    memcpy(shaderInfo->shaderSource->data(), shaderSource.data(), shaderSource.size());
    
    return shaderInfo;
}

//HLSL shader脚本字符串转换
ShaderCodePtr compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType)
{
    return DXCompilerUtil::GetInstance()->compileHLSLToSPIRV(shaderFile, shaderStage, renderType);
}

CompiledShaderInfoPtr CompileShader(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType)
{
    ShaderCodePtr shaderCode = compileHLSLToSPIRV(shaderFile, shaderStage, renderType);
    if (!shaderCode)
    {
        return nullptr;
    }
    
    if (renderType == RenderDeviceType::METAL)
    {
        return compileToMSL(shaderCode, shaderStage);
    }
    
    else if (renderType == RenderDeviceType::VULKAN)
    {
        CompiledShaderInfoPtr compileShader = std::make_shared<CompiledShaderInfo>();
        compileShader->shaderSource = shaderCode;
        return compileShader;
    }
    
    return nullptr;
}

NAMESPACE_SHADERCOMPILER_END
