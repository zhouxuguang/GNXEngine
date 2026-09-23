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
#if GNX_OS_WINDOWS
// DX12 链路专用：D3D12 反射接口（ID3D12ShaderReflection）
#include <d3d12shader.h>
#endif

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

#if GNX_OS_WINDOWS
// DX12(DXIL) 反射：依赖 d3d12shader.h，仅 Windows 编译
bool DXCompilerUtil::reflectDXIL(
    const ShaderCode& bytecode, ShaderStage shaderStage,
    std::vector<RenderCore::CompiledShaderResourceInfo>& resources,
    std::vector<RenderCore::CompiledShaderInputInfo>& inputs)
{
    resources.clear();
    inputs.clear();
    if (!m_pUtils || bytecode.empty())
        return false;

    DxcBuffer buffer = {bytecode.data(), bytecode.size(), DXC_CP_ACP};
    CComPtr<ID3D12ShaderReflection> reflection;
    if (FAILED(m_pUtils->CreateReflection(&buffer, IID_PPV_ARGS(&reflection))) || !reflection)
        return false;

    D3D12_SHADER_DESC shaderDesc = {};
    if (FAILED(reflection->GetDesc(&shaderDesc)))
        return false;

    std::unordered_map<std::string, uint32_t> strides;
    for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i)
    {
        auto* cb = reflection->GetConstantBufferByIndex(i);
        D3D12_SHADER_BUFFER_DESC desc = {};
        if (cb && SUCCEEDED(cb->GetDesc(&desc)) && desc.Name && desc.Size)
            strides[desc.Name] = desc.Size;
    }

    for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC desc = {};
        if (FAILED(reflection->GetResourceBindingDesc(i, &desc)) || !desc.Name)
            continue;

        RenderCore::CompiledShaderResourceInfo info;
        info.name = desc.Name;
        info.binding = desc.BindPoint;
        info.bindCount = desc.BindCount;
        info.dimension = (uint32_t)desc.Dimension;
        switch (desc.Type)
        {
            case D3D_SIT_CBUFFER:
            case D3D_SIT_TBUFFER: info.resourceClass = RenderCore::ShaderResourceClass::CBV; break;
            case D3D_SIT_SAMPLER: info.resourceClass = RenderCore::ShaderResourceClass::Sampler; break;
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                info.resourceClass = RenderCore::ShaderResourceClass::UAV; break;
            default: info.resourceClass = RenderCore::ShaderResourceClass::SRV; break;
        }
        info.isRawBuffer = desc.Type == D3D_SIT_BYTEADDRESS ||
                           desc.Type == D3D_SIT_UAV_RWBYTEADDRESS;
        if (desc.Type == D3D_SIT_STRUCTURED || desc.Type == D3D_SIT_UAV_RWSTRUCTURED)
        {
            auto it = strides.find(info.name);
            if (it == strides.end() && info.name.compare(0, 5, "type_") == 0)
                it = strides.find(info.name.substr(5));
            if (it != strides.end()) info.structuredStride = it->second;
        }
        resources.push_back(std::move(info));
    }

    if (shaderStage == ShaderStage_Vertex)
    {
        for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC desc = {};
            if (SUCCEEDED(reflection->GetInputParameterDesc(i, &desc)) &&
                desc.SystemValueType == D3D_NAME_UNDEFINED && desc.SemanticName)
            {
                inputs.push_back({desc.SemanticName, desc.SemanticIndex, desc.Register});
            }
        }
    }
    return true;
}
#endif  // GNX_OS_WINDOWS

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
            return L"TS";
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
    arguments.push_back(L"-fspv-target-env=vulkan1.2");
    // 保留入口点的全部 interface 变量（含未被静态使用的输入）。
    //
    // 为什么必须做：像素着色器里未被使用的 varying（如 ModelShader 的 PS 只用到
    // texcoord，normal/lightDir/viewDir 都被优化掉）会被 DXC 的 SPIR-V 后端 DCE。
    // 于是 SPIRV-Cross 生成的 HLSL 中，VS 输出声明 4 个 varying，而 PS 只声明 1 个。
    // D3D 的签名寄存器按"声明顺序"分配（不是按语义下标），
    // 导致同一语义 TEXCOORD1 在 VS 是 register1、在 PS 是 register0，
    // CreateGraphicsPipelineState 直接报
    //   "Vertex Shader - Pixel Shader linkage error: ... mismatched hardware registers"。
    // 保留 interface 后 PS 会声明完整 varying 列表，与 VS 输出的顺序/寄存器一致。
    //
    // 只对 PS 开启：VS 的顶点输入保留与否会改变顶点布局（VertexDesc），
    // 而 mesh 的顶点缓冲槽位（如地球的 0/1/3）依赖 VS 输入被 DCE 后的结果，
    // 因此 VS 维持原行为，避免破坏既有顶点布局约定。
    if (shaderStage == ShaderStage_Fragment)
    {
        arguments.push_back(L"-fspv-preserve-interface");
    }
    
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
    // 打印诊断信息。DXC 始终返回一个诊断缓冲：里面可能只有 warning（编译成功），
    // 也可能是真正的编译错误。因此必须先用 GetStatus 区分成败，
    // 否则"只有 warning"的正常编译也会以 ERROR 级别刷屏。
    //
    CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    result = pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    // Note that d3dcompiler would return null if no errors or warnings are present.
    // IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.

    HRESULT hrStatus;
    result = pResults->GetStatus(&hrStatus);
    const bool compileOk = SUCCEEDED(hrStatus);

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
        const char* buffer = pErrors->GetStringPointer();
        if (compileOk)
        {
            LOG_WARN("Shader compiled with warnings: %s\n", buffer);
        }
        else
        {
            LOG_ERROR("Warnings and Errors: %s\n", buffer);
        }
    }

    //
    // Quit if the compilation failed.
    //
    if (!compileOk)
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

#if GNX_OS_WINDOWS
// 以下为 DX12(DXIL) 链路专用：HLSL -> DXIL 编译
namespace
{
// Runs DXC and returns DXIL; callers may retry with a compatibility language version.
// logAsError == false 表示调用方后面还会用兼容模式重试，本次失败属于预期内，
// 不要以 ERROR 级别报出来（否则一次成功编译也会在控制台留下错误）。
CComPtr<IDxcBlob> RunDXCCompile(IDxcCompiler3* compiler, IDxcUtils* utils,
                                const DxcBuffer& source, std::vector<LPCWSTR>& arguments,
                                ShaderStage shaderStage, const char* tag,
                                bool logAsError = true)
{
    CComPtr<IDxcIncludeHandler> includeHandler = nullptr;
    utils->CreateDefaultIncludeHandler(&includeHandler);

    CComPtr<IDxcResult> results = nullptr;
    const HRESULT hr = compiler->Compile(&source, arguments.data(), (UINT32)arguments.size(),
                                         includeHandler, IID_PPV_ARGS(&results));
    if (FAILED(hr) || !results)
    {
        LOG_ERROR("[%s] DXC compile failed: hr=0x%08X, stage=%d", tag, (unsigned)hr,
                  (int)shaderStage);
        return nullptr;
    }

    CComPtr<IDxcBlobUtf8> errors = nullptr;
    results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

    HRESULT status = E_FAIL;
    results->GetStatus(&status);
    const bool compileOk = SUCCEEDED(status);

    if (errors != nullptr && errors->GetStringLength() != 0)
    {
        // 只有 warning 时不要以 ERROR 级别报出来（见 compileHLSLToSPIRV 的同类处理）
        if (compileOk)
        {
            LOG_WARN("[%s] DXC warnings (stage=%d): %s", tag, (int)shaderStage,
                     errors->GetStringPointer());
        }
        else if (logAsError)
        {
            LOG_ERROR("[%s] DXC diagnostics (stage=%d): %s", tag, (int)shaderStage,
                      errors->GetStringPointer());
        }
        else
        {
            LOG_WARN("[%s] DXC diagnostics (stage=%d, will retry with -HV 2016): %s", tag,
                     (int)shaderStage, errors->GetStringPointer());
        }
    }

    if (!compileOk)
    {
        return nullptr;
    }

    CComPtr<IDxcBlob> shader = nullptr;
    results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr);
    if (shader == nullptr || shader->GetBufferSize() == 0)
    {
        LOG_ERROR("[%s] DXC produced no object (stage=%d)", tag, (int)shaderStage);
        return nullptr;
    }

    LOG_INFO("[%s] DXIL compiled (stage=%d, %llu bytes)", tag, (int)shaderStage,
             (unsigned long long)shader->GetBufferSize());

    return shader;
}

/// HLSL 目标 profile（DXIL 编译用）
LPCWSTR GetTargetProfileForDXIL(ShaderStage stage)
{
    switch (stage)
    {
        // 引擎的 shader 使用了 SM 6.x 特性（StructuredBuffer 的完整语义、
        // 资源绑定显式化等），统一按 6.0 起步；Mesh/Task 需要 6.5。
        case ShaderStage_Vertex:   return L"vs_6_0";
        case ShaderStage_Fragment: return L"ps_6_0";
        case ShaderStage_Compute:  return L"cs_6_0";
        case ShaderStage_Task:     return L"as_6_5";
        case ShaderStage_Mesh:     return L"ms_6_5";
        default:                   return L"ps_6_0";
    }
}
} // namespace

LPCWSTR DXCompilerUtil::GetHLSLEntryPoint(ShaderStage stage)
{
    // SPIRV-Cross preserves these entry-point names.
    switch (stage)
    {
        case ShaderStage_Vertex:   return L"VS";
        case ShaderStage_Fragment: return L"PS";
        case ShaderStage_Compute:  return L"CS";
        case ShaderStage_Task:     return L"TS";
        case ShaderStage_Mesh:     return L"MS";
        default:                   return L"PS";
    }
}

ShaderCodePtr DXCompilerUtil::compileHLSLTextToDXIL(const std::string& hlslSource,
                                                    ShaderStage shaderStage)
{
    if (hlslSource.empty())
    {
        LOG_ERROR("[ShaderCompiler] compileHLSLTextToDXIL: 输入 HLSL 为空");
        return nullptr;
    }

    // 源码直接来自内存（SPIRV-Cross 的输出），不需要写临时文件
    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr = hlslSource.data();
    sourceBuffer.Size = hlslSource.size();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    std::vector<LPCWSTR> arguments;

    // 入口点与目标 profile
    arguments.push_back(L"-E");
    arguments.push_back(GetHLSLEntryPoint(shaderStage));
    arguments.push_back(L"-T");
    arguments.push_back(GetTargetProfileForDXIL(shaderStage));

    arguments.push_back(DXC_ARG_PACK_MATRIX_COLUMN_MAJOR);
    // DX12 的 NDC 与 D3D 一致：Z ∈ [0,1]，与引擎的投影矩阵设置吻合
    arguments.push_back(L"-D");
    arguments.push_back(L"TEXCOORD_FLIP");
    if (ShaderCompilerConfig::UseReverseZ)
    {
        arguments.push_back(L"-D");
        arguments.push_back(L"USE_REVERSE_Z");
    }

    // 首次尝试；失败后用 -HV 2016 重试（见 RunDXCCompile 的说明）。
    // 首次失败不算错误：部分 shader（如 SSRTrace 的 PS 里带 [unroll] 的循环）
    // 只有在兼容模式下才会被 DXC 接受，因此这里必须以 logAsError=false 记录。
    CComPtr<IDxcBlob> pShader = RunDXCCompile(m_pCompiler, m_pUtils, sourceBuffer, arguments,
                                              shaderStage, "DXIL", false);
    if (!pShader)
    {
        arguments.push_back(L"-HV");
        arguments.push_back(L"2016");
        pShader = RunDXCCompile(m_pCompiler, m_pUtils, sourceBuffer, arguments,
                                shaderStage, "DXIL/hv2016");
    }
    if (!pShader)
    {
        return nullptr;
    }

    ShaderCodePtr dxilBuffer = std::make_shared<ShaderCode>();
    dxilBuffer->resize(pShader->GetBufferSize());
    memcpy(dxilBuffer->data(), pShader->GetBufferPointer(), pShader->GetBufferSize());

    return dxilBuffer;
}
#endif  // GNX_OS_WINDOWS

NAMESPACE_SHADERCOMPILER_END
