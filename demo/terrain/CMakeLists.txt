add_executable(terrain
main.cpp
TerrainFrameWork.cpp
TerrainFrameWork.h
)

target_link_libraries(terrain PRIVATE GNXEngine)

# iOS 打包必需：Bundle Identifier + 自定义 Info.plist（含 UILaunchScreen/方向）
if(IOS)
    set_target_properties(terrain PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gnxengine.terrain"
        MACOSX_BUNDLE_BUNDLE_NAME "terrain"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.gnxengine.terrain"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
    )
    # 打包预编译 shader（MSL_iOS 容器）到 Bundle Resources/Shader/
    # 移动端运行时通过 AssetManager::LoadResource("Shader/xxx.msl_ios.gnxasset") 读取，
    # iOS 端这些文件必须随 .app 打进 Bundle Resources（NSBundle 才能读到）。
    # 保持 Shader/ 子目录，和运行时路径一致。
    # iOS app bundle 为扁平结构：.app 目录即资源根（NSBundle resourcePath）。
    # 只拷贝 msl_ios 格式，控制包体积。
    set(TERRAIN_SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../data_asset/Shader")
    set(TERRAIN_BUNDLE_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/terrain.app/Shader")
    file(GLOB TERRAIN_MSL_SHADERS "${TERRAIN_SHADER_SRC}/*.msl_ios.gnxasset")
    add_custom_command(TARGET terrain POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TERRAIN_BUNDLE_SHADER_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TERRAIN_MSL_SHADERS} "${TERRAIN_BUNDLE_SHADER_DIR}"
        COMMENT "Copying precompiled MSL_iOS shaders to iOS bundle Shader/"
    )
endif()

set_property(TARGET terrain PROPERTY FOLDER "demo/terrain")
