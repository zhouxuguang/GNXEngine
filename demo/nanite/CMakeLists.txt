add_executable(nanite 
main.cpp 
NaniteFrameWork.cpp
NaniteFrameWork.h
ClusterSelection.cpp
ClusterSelection.h)

target_link_libraries(nanite GNXEngine BaseLib)

set_property(TARGET nanite PROPERTY FOLDER "demo/nanite")