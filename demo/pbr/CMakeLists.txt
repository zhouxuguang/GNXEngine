add_executable(pbr
main.cpp
PBRFrameWork.cpp
PBRFrameWork.h
)

target_link_libraries(pbr GNXEngine)

set_property(TARGET pbr PROPERTY FOLDER "demo/pbr")
