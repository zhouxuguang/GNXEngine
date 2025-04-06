
set(SHADERCOMPILER_SOURCES )
set(shadercompiler_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/ShaderCompiler)
set(shadercompiler_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/ShaderCompiler)
file(GLOB_RECURSE SHADERCOMPILER_SOURCES CONFIGURE_DEPENDS ${shadercompiler_dir}/*.cpp 
                                                           ${shadercompiler_dir}/*.c 
                                                           ${shadercompiler_dir}/*.h
                                                           ${shadercompiler_dir}/spirv_cross/*.cpp 
                                                           ${shadercompiler_dir}/spirv_cross/*.h)
file(GLOB_RECURSE SHADERCOMPILER_INCLUDES CONFIGURE_DEPENDS ${shadercompiler_include_dir}/*.h)

set(SOURCE_FILES ${SHADERCOMPILER_SOURCES} ${SHADERCOMPILER_INCLUDES})

add_library(shadercompiler STATIC ${SOURCE_FILES})
source_group(TREE ${shadercompiler_dir} FILES ${SHADERCOMPILER_SOURCES})
source_group(TREE ${shadercompiler_include_dir} FILES ${SHADERCOMPILER_INCLUDES})

target_include_directories(shadercompiler PUBLIC ${shadercompiler_include_dir} 
                                                 ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
                                                 ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/thirdparty/DirectXShaderCompiler/include)
target_link_libraries(shadercompiler baselib rendercore spirv_reflect)
