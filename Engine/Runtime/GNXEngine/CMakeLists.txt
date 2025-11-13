
set(GNXEngine_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(GNXEngine_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE GNXEngine_Source CONFIGURE_DEPENDS ${GNXEngine_dir}/*.cpp ${GNXEngine_dir}/*.c ${GNXEngine_dir}/*.h)
file(GLOB_RECURSE GNXEngine_Include CONFIGURE_DEPENDS ${GNXEngine_include_dir}/*.h)

if(APPLE)
   list(APPEND GNXEngine_Source ${GNXEngine_dir}/PlatformWindowMisc.mm)
endif(APPLE)

add_library(GNXEngine SHARED ${GNXEngine_Source} ${GNXEngine_Include})
source_group(TREE ${GNXEngine_dir} PREFIX source FILES ${GNXEngine_Source})
source_group(TREE ${GNXEngine_include_dir} PREFIX include FILES ${GNXEngine_Include})

target_include_directories(GNXEngine PUBLIC ${GNXEngine_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../)

target_link_libraries(GNXEngine PUBLIC BaseLib Allocator ImageCodec MathUtil RenderCore RenderSystem
    ShaderCompiler spirv_reflect tbb AssetProcess ktx glfw SDL2-static)
target_compile_definitions(GNXEngine PRIVATE GNXENGINE_EXPORTS)

if(APPLE)
    target_link_libraries(GNXEngine PRIVATE "-framework QuartzCore" "-framework Metal" "-framework GameController")
    target_compile_options(GNXEngine PRIVATE "-fobjc-arc")
    set_target_properties(GNXEngine PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

if(WIN32)
    target_link_libraries(GNXEngine PRIVATE SDL2main)
endif(WIN32)

set_property(TARGET GNXEngine PROPERTY FOLDER "Runtime")
