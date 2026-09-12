add_executable(atmosphere
main.cpp
AtmosphereFrameWork.cpp
AtmosphereFrameWork.h
)

target_link_libraries(atmosphere GNXEngine AssetManager imgui)

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
