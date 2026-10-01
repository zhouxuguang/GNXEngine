add_executable(atmosphere
main.cpp
AtmosphereFrameWork.cpp
AtmosphereFrameWork.h
AtmospherePreset.cpp
AtmospherePreset.h
)

# 注意：这里链接的是 imgui_headers（仅头文件），不是 imgui 静态库本身。
# imgui 的唯一副本位于 GNXEngine 共享库中；若此处直接链接 imgui 静态库，
# 可执行文件会持有第二份 ImGui 全局上下文（GImGui），导致运行时
# "No current context" 断言（详见 ThirdParty/imgui/CMakeLists.txt）。
target_link_libraries(atmosphere GNXEngine AssetManager imgui_headers)

if(IOS)
    set_target_properties(atmosphere PROPERTIES
        OUTPUT_NAME "GNXEngineAtmosphere"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gnxengine.atmosphere"
        MACOSX_BUNDLE_BUNDLE_NAME "GNXEngine Atmosphere"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.gnxengine.atmosphere"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
    )

    set(ATMOS_ASSET_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../data_asset")
    add_custom_command(TARGET atmosphere POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:atmosphere>/Shader"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ATMOS_ASSET_ROOT}/Shader" "$<TARGET_BUNDLE_DIR:atmosphere>/Shader"
        COMMENT "Copying shared engine shader assets to the iOS bundle"
        VERBATIM
    )
endif()

set_property(TARGET atmosphere PROPERTY FOLDER "demo/atmosphere")

# Demo-owned sky shader is compiled offline for every supported backend.
# The iOS bundle copies these assets with the shared Shader directory above.
if(NOT IOS)
    get_filename_component(ATMOS_DEMO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
    set(ATMOS_DEMO_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/Shader/AtmosphereDemoSky.shader")
    set(ATMOS_DEMO_SHADER_OUTPUTS)
    foreach(ATMOS_FORMAT IN ITEMS msl_macos msl_ios spirv)
        set(ATMOS_OUTPUT "${ATMOS_DEMO_ROOT}/data_asset/Shader/AtmosphereDemo/Sky.${ATMOS_FORMAT}.gnxasset")
        add_custom_command(OUTPUT "${ATMOS_OUTPUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ATMOS_DEMO_ROOT}/data_asset/Shader/AtmosphereDemo"
            COMMAND $<TARGET_FILE:shader_compile> "${ATMOS_DEMO_SHADER}" -a -f ${ATMOS_FORMAT} -o "${ATMOS_OUTPUT}"
            DEPENDS shader_compile "${ATMOS_DEMO_SHADER}"
                "${ATMOS_DEMO_ROOT}/Engine/Shader/built-in/Atmosphere/AtmosphereCommon.hlsl"
                "${ATMOS_DEMO_ROOT}/Engine/Shader/built-in/VolumetricCommon.hlsl"
            VERBATIM)
        list(APPEND ATMOS_DEMO_SHADER_OUTPUTS "${ATMOS_OUTPUT}")
    endforeach()
    add_custom_target(atmosphere_demo_shaders DEPENDS ${ATMOS_DEMO_SHADER_OUTPUTS})
    add_dependencies(atmosphere atmosphere_demo_shaders)
endif()
