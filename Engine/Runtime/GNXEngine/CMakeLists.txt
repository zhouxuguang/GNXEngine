
set(GNXEngine_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(GNXEngine_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE GNXEngine_Source CONFIGURE_DEPENDS ${GNXEngine_dir}/*.cpp ${GNXEngine_dir}/*.c ${GNXEngine_dir}/*.h)
file(GLOB_RECURSE GNXEngine_Include CONFIGURE_DEPENDS ${GNXEngine_include_dir}/*.h)

add_library(GNXEngine SHARED ${GNXEngine_Source} ${GNXEngine_Include})
source_group(TREE ${GNXEngine_dir} PREFIX source FILES ${GNXEngine_Source})
source_group(TREE ${GNXEngine_include_dir} PREFIX include FILES ${GNXEngine_Include})

target_include_directories(GNXEngine PUBLIC ${GNXEngine_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../)

target_link_libraries(GNXEngine PUBLIC BaseLib Allocator ImageCodec MathUtil RenderCore RenderSystem
    ShaderCompiler spirv_reflect tbb AssetProcess ktx glfw)
target_compile_definitions(GNXEngine PRIVATE GNXENGINE_EXPORTS)

set_property(TARGET GNXEngine PROPERTY FOLDER "Runtime")
