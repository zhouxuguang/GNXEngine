add_executable(meshlet
main.cpp
MeshletFrameWork.cpp
MeshletFrameWork.h
)

target_link_libraries(meshlet GNXEngine ShaderCompiler spirv_reflect)

if(APPLE)
    target_link_libraries(meshlet ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/mac/libdxcompiler.dylib)
elseif(WIN32)
    target_link_libraries(meshlet ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/win/dxcompiler.lib)
elseif(LINUX)
    target_link_libraries(meshlet ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/DirectXShaderCompiler/lib/linux/libdxcompiler.so)
endif()

set_property(TARGET meshlet PROPERTY FOLDER "demo/meshlet")
