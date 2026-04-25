//
//  DXCompilerUtil.cpp
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/11.
//

#include "DXCompilerUtil.h"
#include "spirv_reflection.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <assert.h>
#include <filesystem>

//代码可以参考这个博客。https://simoncoenen.com/blog/programming/graphics/DxcCompiling

NAMESPACE_SHADERCOMPILER_BEGIN

DXCompilerUtil::DXCompilerUtil()
{
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler));
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_pUtils));
}

DXCompilerUtil::~DXCompilerUtil()
{
    //
}

DXCompilerUtil* DXCompilerUtil::GetInstance()
{
    static DXCompilerUtil instance;
    return &instance;
}

int SpirvReflectExample(const void* spirv_code, size_t spirv_nbytes)
{
    // Generate reflection data for a shader
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_nbytes, spirv_code, &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Enumerate and extract shader's input variables
    uint32_t var_count = 0;
    result = spvReflectEnumerateInputVariables(&module, &var_count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    SpvReflectInterfaceVariable** input_vars =
    (SpvReflectInterfaceVariable**)malloc(var_count * sizeof(SpvReflectInterfaceVariable*));
    result = spvReflectEnumerateInputVariables(&module, &var_count, input_vars);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    //spvReflectEnumerateDescriptorSets(, , )

    // Output variables, descriptor bindings, descriptor sets, and push constants
    // can be enumerated and extracted using a similar mechanism.

    // 释放分配的内存
    free(input_vars);

    // Destroy the reflection data when no longer required.
    spvReflectDestroyShaderModule(&module);
    
    return 0;
}

LPCWSTR GetEntryPoint(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage_Vertex:
            return L"VS";
            break;
            
        case ShaderStage_Fragment:
            return L"PS";
            break;
            
        case ShaderStage_Compute:
            return L"CS";
            break;
            
        case ShaderStage_Task:
            return L"AS";
            break;
            
        case ShaderStage_Mesh:
            return L"MS";
            break;
            
        default:
            break;
    }
    
    return L"PS";
}

LPCWSTR GetTargetProfile(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage_Vertex:
            return L"vs_6_7";
            break;
            
        case ShaderStage_Fragment:
            return L"ps_6_7";
            break;
            
        case ShaderStage_Compute:
            return L"cs_6_7";
            break;
            
        case ShaderStage_Task:
            return L"as_6_7";
            break;
            
        case ShaderStage_Mesh:
            return L"ms_6_7";
            break;
            
        default:
            break;
    }
    
    return L"";
}

static std::wstring GetWC(const std::string& s)
{
    return std::filesystem::path(s).wstring();
}

ShaderCodePtr DXCompilerUtil::compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType)
{
    std::wstring wShaderFile = GetWC(shaderFile);
    
    std::vector<LPCWSTR> arguments;
    arguments.push_back(wShaderFile.c_str());
    
    //-E for the entry point (eg. PSMain)
    arguments.push_back(L"-E");
    arguments.push_back(GetEntryPoint(shaderStage));
    
    //生成spirv的格式的二进制
    arguments.push_back(L"-spirv");

    //-T for the target profile (eg. ps_6_2)
    arguments.push_back(L"-T");
    arguments.push_back(GetTargetProfile(shaderStage));

    //Strip reflection data and pdbs (see later)
    arguments.push_back(L"-Qstrip_debug");
    //arguments.push_back(L"-Qstrip_reflect");

    //arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
    arguments.push_back(DXC_ARG_DEBUG); //-Zi
    arguments.push_back(DXC_ARG_PACK_MATRIX_COLUMN_MAJOR);   //列优先矩阵
    //arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp
    //arguments.push_back(L"-enable-16bit-types");
    
    arguments.push_back(L"-D");
    arguments.push_back(L"TEXCOORD_FLIP");

    // 根据 Reverse-Z 配置添加相应的宏定义
    if (ShaderCompilerConfig::UseReverseZ)
    {
        arguments.push_back(L"-D");
        arguments.push_back(L"USE_REVERSE_Z");
    }

    // -fvk-use-dx-layout -fvk-s-shift 1
    // -fvk-auto-shift-bindings
    
    // 加载shader源码
    CComPtr<IDxcBlobEncoding> pSource = nullptr;
    HRESULT result = m_pUtils->LoadFile(wShaderFile.c_str(), nullptr, &pSource);
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = pSource->GetBufferPointer();
    sourceBuffer.Size = pSource->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;
    
    //
    // 创建默认的IDxcIncludeHandler
    //
    CComPtr<IDxcIncludeHandler> pIncludeHandler = nullptr;
    result = m_pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
    
    CComPtr<IDxcResult> pResults = nullptr;
    result = m_pCompiler->Compile(&sourceBuffer, arguments.data(), (UINT32)arguments.size(), pIncludeHandler, IID_PPV_ARGS(&pResults));
    
    //
    // 如果有错误，就打印出来看看
    //
    CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    result = pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    // Note that d3dcompiler would return null if no errors or warnings are present.
    // IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
        const char* buffer = pErrors->GetStringPointer();
        LOG_ERROR("Warnings and Errors: %s\n", buffer);
    }

    //
    // Quit if the compilation failed.
    //
    HRESULT hrStatus;
    result = pResults->GetStatus(&hrStatus);
    if (FAILED(hrStatus))
    {
		const char* buffer = pErrors->GetStringPointer();
        LOG_ERROR("Compilation Failed: %s\n", buffer);
        return nullptr;
    }
    
    //
    // Save shader binary.
    //
    CComPtr<IDxcBlob> pShader = nullptr;
    CComPtr<IDxcBlobWide> pShaderName = nullptr;
    result = pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);
    if (pShader != nullptr)
    {
        ShaderCodePtr spirvBuffer = std::make_shared<ShaderCode>();
        spirvBuffer->resize(pShader->GetBufferSize());
        memcpy(spirvBuffer->data(), pShader->GetBufferPointer(), pShader->GetBufferSize());
        
        SpirvReflectExample(pShader->GetBufferPointer(), pShader->GetBufferSize());
        
        return spirvBuffer;
    }
    
    return nullptr;
}

NAMESPACE_SHADERCOMPILER_END
