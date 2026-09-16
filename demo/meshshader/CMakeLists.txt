add_executable(meshshader
main.cpp
MeshShaderFrameWork.cpp
MeshShaderFrameWork.h
)

target_link_libraries(meshshader GNXEngine)

set_property(TARGET meshshader PROPERTY FOLDER "demo/meshshader")
