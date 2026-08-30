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
        # 签名：个人开发者团队 (672218231@qq.com / Team 54N72SDZTW)，
        # CODE_SIGN_STYLE=Automatic 让 Xcode 自动选择该 Team 的证书/描述文件。
        #XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "54N72SDZTW"
        #XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    )
    # 打包整个 data_asset 到 Bundle 资源根（与 Android 端 assets.srcDirs 语义一致）：
    #   data_asset/xxx → <bundle 根>/xxx（保留子目录结构）
    # 移动端运行时通过 AssetManager::LoadResource("Shader/xxx.msl_ios.gnxasset")
    # 读取，SDL 会把相对路径解析到 NSBundle resourcePath（.app 根）下的同名路径。
    # 这样 shader、纹理、模型等资源与 Android 一样按 data_asset 原始布局打包，
    # 后续新增 data_asset 子目录（terrain/、pbr/ 等）无需改 CMake。
    # 脚本只保留 msl_ios 合并容器 + 非 shader 资源，控制包体积。
    # 用 $<TARGET_BUNDLE_DIR:terrain> 生成器表达式定位 bundle 目录，
    # 兼容 Xcode（.../Debug-iphoneos/terrain.app）与 Ninja（.../terrain.app）生成器，
    # 避免旧写法 ${CMAKE_CFG_INTDIR} 在 Ninja 下展开为 "." 导致路径错位。
    set(TERRAIN_ASSET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../data_asset")
    # 可执行文件名：bundle 根下同名文件必须保留（不能被 data_asset/terrain/ 资源目录覆盖）
    set(TERRAIN_BUNDLE_PRESERVE "terrain")
    add_custom_command(TARGET terrain POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            "-DSRC_DIR=${TERRAIN_ASSET_SRC}"
            "-DDST_DIR=$<TARGET_BUNDLE_DIR:terrain>"
            "-DPRESERVE_NAMES=${TERRAIN_BUNDLE_PRESERVE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/copy_ios_assets.cmake"
        COMMENT "Copying data_asset to iOS bundle (as packaged resources)"
        VERBATIM
    )
endif()

set_property(TARGET terrain PROPERTY FOLDER "demo/terrain")
