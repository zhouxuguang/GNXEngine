add_executable(terrain
main.cpp
TerrainFrameWork.cpp
TerrainFrameWork.h
)

target_link_libraries(terrain PRIVATE GNXEngine)

# iOS 打包必需：Bundle Identifier
if(IOS)
    set_target_properties(terrain PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gnxengine.terrain"
        MACOSX_BUNDLE_BUNDLE_NAME "terrain"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.gnxengine.terrain"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
    )
endif()

set_property(TARGET terrain PROPERTY FOLDER "demo/terrain")
