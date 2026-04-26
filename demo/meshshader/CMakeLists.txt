add_executable(meshshader
main.cpp
MeshShaderFrameWork.cpp
MeshShaderFrameWork.h
)

target_link_libraries(meshshader GNXEngine ShaderCompiler spirv_reflect)

if(APPLE)
    target_link_libraries(meshshader ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/mac/libdxcompiler.dylib)
elseif(WIN32)
    target_link_libraries(meshshader ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/win/dxcompiler.lib)
elseif(LINUX)
    target_link_libraries(meshshader ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/linux/libdxcompiler.so)
endif()

set_property(TARGET meshshader PROPERTY FOLDER "demo/meshshader")
