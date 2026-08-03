add_subdirectory(meshlet_gen)

# shader_compile is a host-only tool (macOS/Windows); skip on Android/iOS
if(NOT ANDROID AND NOT IOS)
    add_subdirectory(shader_compile)
endif()
