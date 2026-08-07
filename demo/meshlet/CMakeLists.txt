add_executable(meshlet
main.cpp
MeshletFrameWork.cpp
MeshletFrameWork.h
)

target_link_libraries(meshlet GNXEngine ShaderCompiler spirv_reflect)

if(APPLE AND NOT IOS)
    target_link_libraries(meshlet ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/mac/libdxcompiler.dylib)
elseif(WIN32)
    target_link_libraries(meshlet ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/win/dxcompiler.lib)
elseif(LINUX AND NOT ANDROID)
    target_link_libraries(meshlet ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/linux/libdxcompiler.so)
endif()

set_property(TARGET meshlet PROPERTY FOLDER "demo/meshlet")
