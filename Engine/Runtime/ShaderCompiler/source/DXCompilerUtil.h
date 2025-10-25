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
    
private:
    DXCompilerUtil();
    
    ~DXCompilerUtil();
    
    CComPtr<IDxcCompiler3> m_pCompiler = nullptr;
    CComPtr<IDxcUtils> m_pUtils = nullptr;
};


NAMESPACE_SHADERCOMPILER_END

#endif /* GNX_ENGINE_DX_COMPILER_INCLUDESJGVH_H */
