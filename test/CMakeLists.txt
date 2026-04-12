

add_executable(TestMeshImport TestMeshImport.cpp)

target_link_libraries(TestMeshImport PRIVATE GNXEngine ktx)

set_property(TARGET TestMeshImport PROPERTY FOLDER "test")

add_executable(TestModel TestModel.cpp)

target_link_libraries(TestModel GNXEngine)

set_property(TARGET TestModel PROPERTY FOLDER "test")

add_executable(AssetFileHeaderTest AssetFileHeaderTest.cpp)

target_link_libraries(AssetFileHeaderTest PRIVATE GNXEngine)

set_property(TARGET AssetFileHeaderTest PROPERTY FOLDER "test")

add_executable(YamlTest YamlTest.cpp)

target_link_libraries(YamlTest PRIVATE yaml-cpp)

set_property(TARGET YamlTest PROPERTY FOLDER "test")

add_executable(TestHDR2Cubemap TestHDR2Cubemap.cpp)

target_link_libraries(TestHDR2Cubemap PRIVATE GNXEngine)

set_property(TARGET TestHDR2Cubemap PROPERTY FOLDER "test")

# BRDF LUT 离线生成工具（输出 KTX 格式）
add_executable(TestBRDFLUT TestBRDFLUT.cpp)

target_link_libraries(TestBRDFLUT PRIVATE GNXEngine ktx)

set_property(TARGET TestBRDFLUT PROPERTY FOLDER "test")
