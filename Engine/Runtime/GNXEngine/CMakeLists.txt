
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

# DirectX Shader Compiler 运行时库拷贝（参考 RenderCore 里的 CopyAftermathDLL）：
# GNXEngine 通过 RenderSystem / ShaderCompiler 链接了 dxcompiler 导入库，
# 生成的 GNXEngine 共享库在加载时会隐式依赖 dxcompiler.dll（.dylib / .so），
# 必须把它一并拷贝到可执行文件所在目录，否则运行/调试时报
# "找不到 dxcompiler.dll / 0xc000007b"。
# 注意：Android / iOS 使用离线编译的 shader 资产，不链接 DXC，无需拷贝。
if(WIN32 OR APPLE OR (LINUX AND NOT ANDROID))
    # 用 CMAKE_CURRENT_SOURCE_DIR 逐级上溯到仓库根，避免 PROJECT_SOURCE_DIR
    # 在 Android jni 子工程里指向错误位置。
    get_filename_component(GNX_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
    if(WIN32)
        set(DXC_RUNTIME_LIB "${GNX_REPO_ROOT}/ThirdParty/DirectXShaderCompiler/lib/win/dxcompiler.dll")
    elseif(APPLE AND NOT IOS)
        set(DXC_RUNTIME_LIB "${GNX_REPO_ROOT}/ThirdParty/DirectXShaderCompiler/lib/mac/libdxcompiler.dylib")
    elseif(LINUX)
        set(DXC_RUNTIME_LIB "${GNX_REPO_ROOT}/ThirdParty/DirectXShaderCompiler/lib/linux/libdxcompiler.so")
    endif()
    if(EXISTS "${DXC_RUNTIME_LIB}")
        add_custom_command(TARGET GNXEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DXC_RUNTIME_LIB}"
                "$<TARGET_FILE_DIR:GNXEngine>"
            COMMENT "Copying ${DXC_RUNTIME_LIB} to $<TARGET_FILE_DIR:GNXEngine>"
        )
    else()
        message(WARNING "DXC runtime library not found: ${DXC_RUNTIME_LIB}")
    endif()
endif()

set_property(TARGET GNXEngine PROPERTY FOLDER "Runtime")
