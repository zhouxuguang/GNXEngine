# Dear ImGui - 即时模式 GUI 库（核心，不含 backends）
#
# 版本：v1.92.9b（https://github.com/ocornut/imgui/releases/tag/v1.92.9b）
#
# 1.92 起采用动态字体图集（ImGuiBackendFlags_RendererHasTextures）：
# 字形按需光栅化，无需预设字形范围；后端需自行响应 ImDrawData::Textures 的
# 创建/增量更新/销毁请求（见 Engine/Runtime/RenderSystem/include/UI/ImGuiRenderer.cpp）。
#
# 说明：
#   1) 只编译核心必要源文件，官方 backends/（imgui_impl_glfw / imgui_impl_sdl2 /
#      imgui_impl_vulkan / imgui_impl_metal）与 examples/、misc/ 均不引入。
#      平台输入与渲染后端由引擎自行实现（见 doc/ImGuiIntegrationDesign.md）。
#   2) 核心库不依赖 GLFW/SDL 等窗口系统，依赖干净。
#   3) 编译为【静态库】。
#
# 静态化后的关键约束 —— 一个进程内只能有一份 ImGui 代码：
#   ImGui 的全局上下文 GImGui 是「每个镜像（dylib / exe）各自一份」的静态变量。
#   若引擎共享库与可执行文件各静态链接一份 imgui，两者互不可见，
#   运行时会触发 ImGui::GetIO() 的 "No current context" 断言。
#   因此本项目约定：
#     * 由 GNXEngine 共享库承载 imgui 的【唯一副本】并导出全部 ImGui 符号
#       （见 Engine/Runtime/GNXEngine/CMakeLists.txt 的 whole-archive / force_load）；
#     * 可执行文件不链接本静态库，只链接 imgui_headers 取头文件，
#       ImGui 符号从引擎库导入（见 demo/*/CMakeLists.txt）。
#     * 移动端（iOS/Android）整体静态链接为单一二进制，本来就只有一份，天然满足。

project(imgui LANGUAGES CXX)

set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)

set(IMGUI_HEADERS
    imgui/imgui.h
    imgui/imgui_internal.h
    imgui/imconfig.h
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})

# 头文件查找路径：使用方 #include "imgui.h" 即可
target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# 「只需要头文件、不需要链接静态库」的消费者使用（demo / 编辑器等）：
# 它们的 ImGui 符号应当从 GNXEngine 共享库导入，
# 直接链接 imgui 静态库会在进程内产生第二份 GImGui。
add_library(imgui_headers INTERFACE)
target_include_directories(imgui_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

if(WIN32)
    # Windows 下静态库会被并入 GNXEngine.dll，符号必须显式导出/导入
    # （macOS/Linux 使用默认可见性，共享库会自动导出，无需额外处理）。
    target_compile_definitions(imgui PRIVATE IMGUI_API=__declspec(dllexport))
    target_compile_definitions(imgui_headers INTERFACE IMGUI_API=__declspec(dllimport))
endif()

# 产物名统一（不带 Debug 后缀）：
# Xcode 生成器跨目录引用时使用 libimgui.a，若产物带 d 后缀会链接失败。
#
# 可见性：工程根 CMakeLists 全局设置了 CMAKE_CXX_VISIBILITY_PRESET=hidden，
# 而 imgui 内部没有导出宏；若沿用 hidden，并入共享库后不会导出任何 ImGui 符号
# （可执行文件链接时报未定义符号），因此这里单独覆盖为 default。
#
# POSITION_INDEPENDENT_CODE：本静态库会被并入共享库（dylib/so），必须开启 PIC。
set_target_properties(imgui PROPERTIES
    OUTPUT_NAME imgui
    DEBUG_POSTFIX ""
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET default
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)

# IDE 分组：与其它第三方库（glfw / ktx / meshoptimizer / tbb ...）保持一致，
# 统一归入 ThirdParty 分组。
#   - FOLDER      ：解决方案 / 工程视图中的「目标」分组（仓库内 13 个第三方库均使用该写法）
#   - source_group：IDE 中「源文件」筛选器分组，保持 ThirdParty/imgui/... 层级
set_property(TARGET imgui PROPERTY FOLDER "ThirdParty")
