add_executable(shader_compile
    main.cpp
)

# 工具通过 ShaderPackageBuilder 打包（AssetManager 内部处理 pb），无需直接链接 nanopb
target_link_libraries(shader_compile PRIVATE GNXEngine)

set_property(TARGET shader_compile PROPERTY FOLDER "Tool")
