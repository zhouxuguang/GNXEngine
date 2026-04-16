add_executable(terrain
main.cpp
TerrainFrameWork.cpp
TerrainFrameWork.h
)

target_link_libraries(terrain GNXEngine)

set_property(TARGET terrain PROPERTY FOLDER "demo/terrain")
