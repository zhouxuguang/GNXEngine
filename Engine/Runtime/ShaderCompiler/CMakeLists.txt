
set(SHADERCOMPILER_SOURCES )
set(shadercompiler_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(shadercompiler_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE SHADERCOMPILER_SOURCES CONFIGURE_DEPENDS ${shadercompiler_dir}/*.cpp 
                                                           ${shadercompiler_dir}/*.c 
                                                           ${shadercompiler_dir}/*.h
                                                           ${shadercompiler_dir}/spirv_cross/*.cpp 
                                                           ${shadercompiler_dir}/spirv_cross/*.h)
file(GLOB_RECURSE SHADERCOMPILER_INCLUDES CONFIGURE_DEPENDS ${shadercompiler_include_dir}/*.h)

set(SOURCE_FILES ${SHADERCOMPILER_SOURCES} ${SHADERCOMPILER_INCLUDES})

# Android/iOS use precompiled shaders; exclude DXC host tool
if(ANDROID OR IOS)
    list(FILTER SOURCE_FILES EXCLUDE REGEX ".*DXCompilerUtil\\.(cpp|h)$")
endif()

# SPIRV-Cross 的 HLSL 后端只服务于 DX12(DXIL) 链路，非 Windows 平台不需要
if(NOT WIN32)
    list(FILTER SOURCE_FILES EXCLUDE REGEX ".*spirv_hlsl\\.(cpp|h)$")
endif()

add_library(ShaderCompiler OBJECT ${SOURCE_FILES})
source_group(TREE ${shadercompiler_dir} PREFIX source FILES ${SHADERCOMPILER_SOURCES})
source_group(TREE ${shadercompiler_include_dir} PREFIX include FILES ${SHADERCOMPILER_INCLUDES})

# DXC include 目录：HLSL -> SPIR-V 这条链路在 Windows/macOS/Linux 的离线编译中都要用到，
# 因此不按平台裁剪；DX12(DXIL) 专属部分由源码内的 GNX_OS_WINDOWS 隔离。
target_include_directories(ShaderCompiler PRIVATE ${shadercompiler_include_dir} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/DirectXShaderCompiler/include)
target_link_libraries(ShaderCompiler PRIVATE BaseLib RenderCore spirv_reflect)

set_property(TARGET ShaderCompiler PROPERTY FOLDER "Runtime")