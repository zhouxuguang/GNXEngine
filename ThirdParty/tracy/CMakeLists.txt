cmake_minimum_required(VERSION 3.17)

# Tracy profiler client library (only the instrumentation client, not the GUI)
# Enable profiling by setting -DENABLE_TRACY=ON at CMake configure time

add_library(TracyClient STATIC
    TracyClient.cpp
)

target_include_directories(TracyClient PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tracy
)

# Tracy requires C++17
target_compile_features(TracyClient PUBLIC cxx_std_17)

# When TRACY_ENABLE is defined, Tracy instruments the code; otherwise all macros are no-ops
if(ENABLE_TRACY)
    target_compile_definitions(TracyClient PUBLIC TRACY_ENABLE)
endif()

# Platform-specific link dependencies for Tracy
if(WIN32)
    target_link_libraries(TracyClient PUBLIC ws2_32 advapi32 user32 dbghelp)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(TracyClient PUBLIC pthread dl)
endif()

# Suppress warnings from Tracy source
if(MSVC)
    target_compile_options(TracyClient PRIVATE /W0)
else()
    target_compile_options(TracyClient PRIVATE -w)
endif()

set_property(TARGET TracyClient PROPERTY FOLDER "ThirdParty")
