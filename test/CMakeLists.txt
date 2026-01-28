

add_executable(TestMeshImport TestMeshImport.cpp)

target_link_libraries(TestMeshImport PRIVATE GNXEngine ktx)

set_property(TARGET TestMeshImport PROPERTY FOLDER "test")

add_executable(TestModel TestModel.cpp)

target_link_libraries(TestModel GNXEngine)

set_property(TARGET TestModel PROPERTY FOLDER "test")

add_executable(AssetFileHeaderTest AssetFileHeaderTest.cpp)

target_link_libraries(AssetFileHeaderTest PRIVATE GNXEngine)

set_property(TARGET AssetFileHeaderTest PROPERTY FOLDER "test")
