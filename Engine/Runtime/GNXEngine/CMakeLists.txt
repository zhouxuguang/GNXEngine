
set(GNXEngine_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(GNXEngine_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================
# 平台窗口后端选择
#   Android / iOS  → SDL2  (GNX_WINDOW_SDL=1)
#   Windows / macOS / Linux → GLFW (GNX_WINDOW_SDL=0)
# ============================================================
if(ANDROID OR IOS)
    set(GNX_WINDOW_SDL 1)
    set(GNX_WINDOW_SDL_DEFINE "GNX_WINDOW_SDL=1")
else()
    set(GNX_WINDOW_SDL 0)
    set(GNX_WINDOW_SDL_DEFINE "GNX_WINDOW_SDL=0")
endif()

# 收集源文件
file(GLOB_RECURSE GNXEngine_Source CONFIGURE_DEPENDS ${GNXEngine_dir}/*.cpp ${GNXEngine_dir}/*.c ${GNXEngine_dir}/*.h)
file(GLOB_RECURSE GNXEngine_Include CONFIGURE_DEPENDS ${GNXEngine_include_dir}/*.h)

# Apple 平台额外添加 ObjC 桥接文件
if(APPLE)
   list(APPEND GNXEngine_Source ${GNXEngine_dir}/PlatformWindowMisc.mm)
endif(APPLE)

if(IOS)
    add_library(GNXEngine STATIC ${GNXEngine_Source} ${GNXEngine_Include})
else()
    add_library(GNXEngine SHARED ${GNXEngine_Source} ${GNXEngine_Include})
endif()
source_group(TREE ${GNXEngine_dir} PREFIX source FILES ${GNXEngine_Source})
source_group(TREE ${GNXEngine_include_dir} PREFIX include FILES ${GNXEngine_Include})

target_include_directories(GNXEngine PUBLIC ${GNXEngine_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../)

# 窗口后端链接
if(GNX_WINDOW_SDL)
    # SDL2 路径（Android / iOS）
    target_link_libraries(GNXEngine PRIVATE SDL2-static SDL2main)
else()
    # GLFW 路径（Windows / macOS / Linux）
    target_link_libraries(GNXEngine PRIVATE glfw)
endif()

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
    TracyClient
)

# 窗口后端宏定义（控制 #if GNX_WINDOW_SDL）
# 注意：GNX_WINDOW_SDL 必须是 PUBLIC，demo 的 main.cpp 通过 GNXMain.h
# 依赖该宏决定是否把 main 重命名为 SDL_main（iOS/Android 必需，
# 否则 SDL_Init 会报 "did you include SDL_main.h"）。
target_compile_definitions(GNXEngine PUBLIC ${GNX_WINDOW_SDL_DEFINE})
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
elseif(ANDROID)
    # ANDROID使用 --whole-archive
    # target_link_libraries(GNXEngine PRIVATE 
    #     -Wl,--whole-archive
    #     ISPCCompress
    #     -Wl,--no-whole-archive
    # )
elseif(MSVC)
    # Windows 使用 /WHOLEARCHIVE
    target_link_options(GNXEngine PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:ISPCCompress>")
endif()

if(APPLE)
    target_link_libraries(GNXEngine PRIVATE "-framework QuartzCore" "-framework Metal" "-framework GameController"
        "-framework CoreGraphics" "-framework ImageIO" "-framework CoreServices" "-framework Foundation")
    target_compile_options(GNXEngine PRIVATE "-fobjc-arc")
    set_target_properties(GNXEngine PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

# 16 KB 页大小兼容（Android 15+ / Google Play 要求）：
# NDK 26 工具链默认 max-page-size=4096，这里强制把 LOAD 段对齐到 16384，
# 否则 APK 无法在 16 KB 页大小设备上运行。
if(ANDROID)
    target_link_options(GNXEngine PRIVATE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384"
    )
endif()

set_property(TARGET GNXEngine PROPERTY FOLDER "Runtime")
