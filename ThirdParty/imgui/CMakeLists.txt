# Dear ImGui - 即时模式 GUI 库（核心，不含 backends）
#
# 版本：v1.91.9b（https://github.com/ocornut/imgui/releases/tag/v1.91.9b）
# 许可证：MIT，见 imgui/LICENSE.txt
#
# 说明：
#   1) 只编译核心必要源文件，官方 backends/（imgui_impl_glfw / imgui_impl_sdl2 /
#      imgui_impl_vulkan / imgui_impl_metal）与 examples/、misc/ 均不引入。
#      平台输入与渲染后端由引擎自行实现（见 doc/ImGuiIntegrationDesign.md）。
#   2) 核心库不依赖 GLFW/SDL 等窗口系统，依赖干净。

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

# 关键：桌面平台引擎本身是动态库。若 imgui 用静态库，「引擎 dylib」与「可执行文件」
# 会各自静态链接一份 imgui，从而各持有一份 ImGui 的全局上下文变量 GImGui，
# 运行时互相看不见（表现为 ImGui::GetIO() 断言 "No current context"）。
# 因此桌面平台编译为共享库，保证进程内只有一份 ImGui 上下文。
# 移动端（iOS/Android）整体静态链接为单一二进制，使用静态库即可。
if(ANDROID OR IOS)
    add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
else()
    add_library(imgui SHARED ${IMGUI_SOURCES} ${IMGUI_HEADERS})
    if(WIN32)
        target_compile_definitions(imgui PRIVATE IMGUI_API=__declspec(dllexport))
        target_compile_definitions(imgui INTERFACE IMGUI_API=__declspec(dllimport))
    endif()
endif()

# 头文件查找路径：使用方 #include "imgui.h" 即可
# 统一产物名（不带 Debug 后缀），避免 Xcode 生成器在跨目录引用共享库时
# 使用 libimgui.dylib 而实际产物是 libimguid.dylib 导致链接失败
#
# 可见性：工程根 CMakeLists 全局设置了 CMAKE_CXX_VISIBILITY_PRESET=hidden，
# imgui 内部没有导出宏，若沿用 hidden 则共享库不导出任何 ImGui 符号（链接失败），
# 因此这里单独覆盖为 default。
set_target_properties(imgui PROPERTIES
    OUTPUT_NAME imgui
    DEBUG_POSTFIX ""
    C_VISIBILITY_PRESET default
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)
