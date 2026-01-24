# nlohmann/json - Header-only JSON library
# This is a header-only library, so we just need to export the include path

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Export the library so it can be found by other projects
set_target_properties(nlohmann_json PROPERTIES
    EXPORT_NAME nlohmann_json
)
