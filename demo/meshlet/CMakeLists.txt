add_executable(meshlet
main.cpp
MeshletFrameWork.cpp
MeshletFrameWork.h
)

target_link_libraries(meshlet GNXEngine)

set_property(TARGET meshlet PROPERTY FOLDER "demo/meshlet")
