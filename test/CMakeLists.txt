

add_executable(TestMeshImport TestMeshImport.cpp)

target_link_libraries(TestMeshImport PRIVATE GNXEngine Allocator)

if(APPLE)
    target_link_libraries(TestMeshImport PRIVATE "-framework CoreGraphics" "-framework CoreVideo" "-framework QuartzCore" 
        "-framework GameController" "-framework Metal" "-framework ImageIO" "-framework OpenGL" "-framework AppKit")
    target_compile_options(TestMeshImport PRIVATE "-fobjc-arc")
    set_target_properties(TestMeshImport PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

set_property(TARGET TestMeshImport PROPERTY FOLDER "test")

add_executable(TestModel TestModel.cpp)

target_link_libraries(TestModel GNXEngine spirv_reflect tbb ktx glfw)

set_property(TARGET TestModel PROPERTY FOLDER "test")
