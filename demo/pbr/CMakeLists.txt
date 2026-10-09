add_executable(pbr
main.cpp
PBRFrameWork.cpp
PBRFrameWork.h
)

# 链接 imgui_headers（仅头文件）：ImGui 唯一副本位于 GNXEngine 共享库中，
# 这里只取头文件用于构建 UI，避免可执行文件持有第二份 ImGui 全局上下文。
target_link_libraries(pbr GNXEngine AssetManager imgui_headers)

if(IOS)
    set_target_properties(pbr PROPERTIES
        OUTPUT_NAME "GNXEnginePBR"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gnxengine.pbr"
        MACOSX_BUNDLE_BUNDLE_NAME "GNXEngine PBR"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.gnxengine.pbr"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
    )

    # Mobile resource paths are relative to the application bundle. The executable uses a
    # distinct name so the bundle can also contain the required top-level pbr/ asset folder.
    set(PBR_ASSET_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../data_asset")
    add_custom_command(TARGET pbr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:pbr>/Shader"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:pbr>/pbr"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PBR_ASSET_ROOT}/Shader" "$<TARGET_BUNDLE_DIR:pbr>/Shader"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PBR_ASSET_ROOT}/pbr" "$<TARGET_BUNDLE_DIR:pbr>/pbr"
        COMMENT "Copying PBR and shared engine assets to the iOS bundle"
        VERBATIM
    )
endif()

set_property(TARGET pbr PROPERTY FOLDER "demo/pbr")
