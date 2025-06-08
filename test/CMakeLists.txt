

add_executable(TestMeshImport TestMeshImport.cpp)

target_link_libraries(TestMeshImport baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect AssetProcess)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET TestMeshImport PROPERTY FOLDER "test")

add_executable(TestModel TestModel.cpp)

target_link_libraries(TestModel baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect AssetProcess glfw)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET TestModel PROPERTY FOLDER "test")
