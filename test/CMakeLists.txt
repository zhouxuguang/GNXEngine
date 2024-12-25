

add_executable(TestMeshImport TestMeshImport.cpp)

target_link_libraries(TestMeshImport baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect tbb_static nanopb)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET TestMeshImport PROPERTY FOLDER "test")
