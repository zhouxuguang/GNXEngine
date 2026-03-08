
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

target_link_libraries(GNXEngine PRIVATE 
    nlohmann_json
    ISPCCompress
    AssetManager
    AssetProcess 
    Allocator
    BaseLib
    MathUtil 
    ImageCodec 
    RenderCore 
    RenderSystem 
    ShaderCompiler 
    spirv_reflect 
    tbb 
    ktx 
    glfw
)
target_compile_definitions(GNXEngine PRIVATE 
    GNXENGINE_EXPORTS
    RENDERCORE_EXPORTS
    RENDERSYSTEM_EXPORTS
)

if(APPLE)
    # macOS
    # target_link_libraries(GNXEngine PRIVATE
    #     "-Wl,-force_load,$<TARGET_FILE:ISPCCompress>"
    # )
elseif(UNIX)
    # Linux使用 --whole-archive
    target_link_libraries(GNXEngine PRIVATE 
        -Wl,--whole-archive
        ISPCCompress
        -Wl,--no-whole-archive
    )
elseif(MSVC)
    # Windows 使用 /WHOLEARCHIVE
    target_link_options(GNXEngine PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:ISPCCompress>")
endif()

if(APPLE)
    target_link_libraries(GNXEngine PRIVATE "-framework QuartzCore" "-framework Metal" "-framework GameController")
    target_compile_options(GNXEngine PRIVATE "-fobjc-arc")
    set_target_properties(GNXEngine PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

# if(UNIX)
#     target_link_options(GNXEngine PRIVATE "LINKER:--exclude-libs,ALL")
# elseif(WIN32)
#     target_link_options(GNXEngine PRIVATE "/NODEFAULTLIB")
# endif()

set_property(TARGET GNXEngine PROPERTY FOLDER "Runtime")
