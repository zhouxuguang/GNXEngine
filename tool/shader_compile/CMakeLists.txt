add_executable(shader_compile
    main.cpp
)

# 工具通过 ShaderPackageBuilder 打包（AssetManager 内部处理 pb），无需直接链接 nanopb
target_link_libraries(shader_compile PRIVATE GNXEngine ShaderCompiler spirv_reflect)

if(APPLE)
    target_link_libraries(shader_compile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/mac/libdxcompiler.dylib)
elseif(WIN32)
    target_link_libraries(shader_compile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/win/dxcompiler.lib)
elseif(LINUX)
    target_link_libraries(shader_compile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/linux/libdxcompiler.so)
endif()

set_property(TARGET shader_compile PROPERTY FOLDER "Tool")
