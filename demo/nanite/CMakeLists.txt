add_executable(nanite 
main.cpp 
NaniteFrameWork.cpp
NaniteFrameWork.h)

target_link_libraries(nanite GNXEngine)

set_property(TARGET nanite PROPERTY FOLDER "demo/nanite")