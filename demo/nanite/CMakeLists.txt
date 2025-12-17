add_executable(nanite 
main.cpp
ClusterCull.cpp
ClusterCull.h
NaniteFrameWork.cpp
NaniteFrameWork.h
RasterClear.cpp
RasterClear.h
NodeAndClusterCull.cpp
NodeAndClusterCull.h
SwapChain.cpp 
SwapChain.h
HardwareRasterization.cpp
HardwareRasterization.h
Visualization.cpp
Visualization.h)

target_link_libraries(nanite GNXEngine)

set_property(TARGET nanite PROPERTY FOLDER "demo/nanite")