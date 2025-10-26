set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw)

set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")
set_property(TARGET update_mappings PROPERTY FOLDER "ThirdParty")
