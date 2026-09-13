add_executable(virtualtexture
    main.cpp
    VTFrameWork.cpp
    VTFrameWork.h
)

# 链接 imgui_headers（仅头文件）：ImGui 唯一副本位于 GNXEngine 共享库中，
# 这里只取头文件用于构建 UI，避免可执行文件持有第二份 ImGui 全局上下文。
target_link_libraries(virtualtexture GNXEngine imgui_headers)

set_property(TARGET virtualtexture PROPERTY FOLDER "demo/virtualtexture")