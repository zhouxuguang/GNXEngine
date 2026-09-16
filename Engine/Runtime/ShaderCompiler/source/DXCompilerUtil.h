//
//  DXCompilerUtil.h
//  GNXEngine
//
//  Created by zhouxuguang on 2022/9/11.
//

#ifndef GNX_ENGINE_DX_COMPILER_INCLUDESJGVH_H
#define GNX_ENGINE_DX_COMPILER_INCLUDESJGVH_H

#include "ShaderCompilerDefine.h"
#include "Runtime/RenderCore/include/ShaderFunction.h"
#include "Runtime/RenderCore/include/ShaderStageData.h"

#ifdef _WIN32
    #include <Windows.h>
    #include <atlbase.h>
    #include "dxc/dxcapi.h"
#else
    #define __EMULATE_UUID
    #include "dxc/dxcapi.h"
#endif

NAMESPACE_SHADERCOMPILER_BEGIN

class DXCompilerUtil
{
public:
    static DXCompilerUtil* GetInstance();
    
    ShaderCodePtr compileHLSLToSPIRV(const std::string& shaderFile, ShaderStage shaderStage, RenderDeviceType renderType);

#if GNX_OS_WINDOWS
    // ---- 以下为 DX12(DXIL) 链路专用，依赖 d3d12shader.h 做 DXIL 反射 ----
    /// Compiles generated HLSL text to DXIL.
    ShaderCodePtr compileHLSLTextToDXIL(const std::string& hlslSource, ShaderStage shaderStage);
    bool reflectDXIL(const ShaderCode& bytecode, ShaderStage shaderStage,
                     std::vector<RenderCore::CompiledShaderResourceInfo>& resources,
                     std::vector<RenderCore::CompiledShaderInputInfo>& inputs);

    /// Returns the preserved engine entry-point name for a shader stage.
    static LPCWSTR GetHLSLEntryPoint(ShaderStage stage);
#endif

private:
    DXCompilerUtil();
    
    ~DXCompilerUtil();
    
    CComPtr<IDxcCompiler3> m_pCompiler = nullptr;
    CComPtr<IDxcUtils> m_pUtils = nullptr;
};


NAMESPACE_SHADERCOMPILER_END

#endif /* GNX_ENGINE_DX_COMPILER_INCLUDESJGVH_H */
