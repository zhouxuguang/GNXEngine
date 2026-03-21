add_executable(ssao 
main.cpp
SSAOFrameWork.cpp
SSAOFrameWork.h)

target_link_libraries(ssao GNXEngine)

set_property(TARGET ssao PROPERTY FOLDER "demo/ssao")