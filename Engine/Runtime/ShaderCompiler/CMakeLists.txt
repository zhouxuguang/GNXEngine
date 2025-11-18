
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

add_library(ShaderCompiler OBJECT ${SOURCE_FILES})
source_group(TREE ${shadercompiler_dir} PREFIX source FILES ${SHADERCOMPILER_SOURCES})
source_group(TREE ${shadercompiler_include_dir} PREFIX include FILES ${SHADERCOMPILER_INCLUDES})

target_include_directories(ShaderCompiler PRIVATE ${shadercompiler_include_dir} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ThirdParty/DirectXShaderCompiler/include)
target_link_libraries(ShaderCompiler PRIVATE BaseLib RenderCore spirv_reflect)

set_property(TARGET ShaderCompiler PROPERTY FOLDER "Runtime")