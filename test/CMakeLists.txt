

add_executable(TestMeshImport TestMeshImport.cpp tlsf.c tlsf.h)

target_link_libraries(TestMeshImport PRIVATE baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect AssetProcess tbb)

if(APPLE)
    target_link_libraries(TestMeshImport PRIVATE "-framework CoreGraphics" "-framework CoreVideo" "-framework QuartzCore" 
        "-framework GameController" "-framework Metal" "-framework ImageIO" "-framework OpenGL" "-framework AppKit")
    target_compile_options(TestMeshImport PRIVATE "-fobjc-arc")
    set_target_properties(TestMeshImport PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET TestMeshImport PROPERTY FOLDER "test")

add_executable(TestModel TestModel.cpp)

target_link_libraries(TestModel baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect AssetProcess glfw ktx)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET TestModel PROPERTY FOLDER "test")
