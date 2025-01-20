

add_executable(TestMeshImport TestMeshImport.cpp)

target_link_libraries(TestMeshImport GNXEngine)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET TestMeshImport PROPERTY FOLDER "test")

add_executable(TestModel TestModel.cpp)

target_link_libraries(TestModel GNXEngine glfw)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET TestModel PROPERTY FOLDER "test")
