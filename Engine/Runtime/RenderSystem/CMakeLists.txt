
set(RENDERSYSTEM_SOURCES )
set(rendersystem_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(rendersystem_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(APPLE)
    file(GLOB_RECURSE RENDERSYSTEM_SOURCES CONFIGURE_DEPENDS ${rendersystem_dir}/*.cpp 
                                ${rendersystem_dir}/*.mm
                                ${rendersystem_dir}/*.c 
                                ${rendersystem_dir}/*.h) 
    file(GLOB_RECURSE RENDERSYSTEM_INCLUDES CONFIGURE_DEPENDS ${rendersystem_include_dir}/*.h
                                ${rendersystem_include_dir}/*.cpp
                                ${rendersystem_include_dir}/*.mm
                                ${rendersystem_include_dir}/*.c)
elseif(WIN32)
    file(GLOB_RECURSE RENDERSYSTEM_SOURCES CONFIGURE_DEPENDS ${rendersystem_dir}/*.cpp 
                                ${rendersystem_dir}/*.c 
                                ${rendersystem_dir}/*.h) 
    file(GLOB_RECURSE RENDERSYSTEM_INCLUDES CONFIGURE_DEPENDS ${rendersystem_include_dir}/*.h
                                ${rendersystem_include_dir}/*.cpp
                                ${rendersystem_include_dir}/*.c)
elseif(UNIX AND NOT APPLE)  # linux
    file(GLOB_RECURSE RENDERSYSTEM_SOURCES CONFIGURE_DEPENDS ${rendersystem_dir}/*.cpp 
                                ${rendersystem_dir}/*.c 
                                ${rendersystem_dir}/*.h) 
    file(GLOB_RECURSE RENDERSYSTEM_INCLUDES CONFIGURE_DEPENDS ${rendersystem_include_dir}/*.h
                                ${rendersystem_include_dir}/*.cpp
                                ${rendersystem_include_dir}/*.c)
endif()

set(SOURCE_FILES ${RENDERSYSTEM_SOURCES} ${RENDERSYSTEM_INCLUDES})

add_library(RenderSystem STATIC ${SOURCE_FILES})

source_group(TREE ${rendersystem_dir} PREFIX source FILES ${RENDERSYSTEM_SOURCES})
source_group(TREE ${rendersystem_include_dir} PREFIX include FILES ${RENDERSYSTEM_INCLUDES})

target_include_directories(RenderSystem PRIVATE ${rendersystem_include_dir})

target_include_directories(RenderSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/glm-0.9.9.8)
                                           

#这里后续还需要修改
#target_link_directories(rendersystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/thirdparty/assimp-5.2.4/lib/Mac)
# target_link_directories(rendersystem PRIVATE 
#         /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/lib/Mac
#         /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/DirectXShaderCompiler/lib/mac)

if(APPLE)
target_link_libraries(RenderSystem PRIVATE BaseLib RenderCore ShaderCompiler 
    assimp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/DirectXShaderCompiler/lib/mac/libdxcompiler.dylib)
elseif(WIN32)
target_link_libraries(RenderSystem PRIVATE BaseLib RenderCore ShaderCompiler 
    assimp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/DirectXShaderCompiler/lib/win/dxcompiler.lib)
elseif(LINUX)
target_link_libraries(RenderSystem PRIVATE BaseLib RenderCore ShaderCompiler 
    assimp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/DirectXShaderCompiler/lib/linux/libdxcompiler.so)
endif() 

#/Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib
# target_link_libraries(rendersystem PUBLIC baselib rendercore shadercompiler 
#     ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/thirdparty/assimp-5.2.4/lib/Mac/libassimp.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libdxcompiler.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangIndex.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangCodeGen.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangRewriteFrontend.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangFrontend.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangDriver.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangSema.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangEdit.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangAST.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangCodeGen.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangLex.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangTooling.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangBasic.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclang.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libclangSPIRV.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMHLSL.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMDxcSupport.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMDXIL.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMDxilContainer.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMDxilPIXPasses.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMDxilRootSignature.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMDxrFallback.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMInstCombine.a
#     /Users/zhouxuguang/work/mycode/DirectXShaderCompiler/build/lib/libLLVMMSSupport.a)

# clangIndex
# #  clangARCMigrate
#   clangRewrite
#   clangCodeGen
#   clangRewriteFrontend
#   clangFrontend
#   clangDriver
# #  clangSerialization
#   clangSema
#   clangEdit
#   clangAST
#   clangCodeGen
#   clangLex
#   clangTooling
#   clangBasic
#   libclang


