add_executable(lumen 
main.cpp
LumenFrameWork.cpp
LumenFrameWork.h)

target_link_libraries(lumen GNXEngine)

set_property(TARGET lumen PROPERTY FOLDER "demo/lumen")