add_executable(shader_compile
    main.cpp
)

target_link_libraries(shader_compile PRIVATE
    AssetProcess
    AssetManager
    ShaderCompiler
    BaseLib
    MathUtil
    ImageCodec
    RenderCore
    RenderSystem
    nanopb
    ISPCCompress
    tbb
    ktx
    spirv_reflect
)

# DirectXShaderCompiler (DXC) 用于 HLSL → SPIR-V
target_link_directories(shader_compile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/mac
)
target_link_libraries(shader_compile PRIVATE
    dxcompiler
)

set_property(TARGET shader_compile PROPERTY FOLDER "Tool")
