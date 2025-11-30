add_executable(nanite 
main.cpp 
NaniteFrameWork.cpp
NaniteFrameWork.h
RasterClear.cpp
RasterClear.h
ClusterSelection.cpp
ClusterSelection.h
SwapChain.cpp 
SwapChain.h
HardwareRasterization.cpp
HardwareRasterization.h
Visualization.cpp
Visualization.h)

target_link_libraries(nanite GNXEngine)

set_property(TARGET nanite PROPERTY FOLDER "demo/nanite")