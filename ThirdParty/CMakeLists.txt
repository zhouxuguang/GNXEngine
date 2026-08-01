
add_subdirectory(nlohmann nlohmann)
add_subdirectory(assimp assimp)
add_subdirectory(SPIRV-Reflect SPIRV-Reflect)
add_subdirectory(tbb tbb)
add_subdirectory(meshoptimizer meshoptimizer)
add_subdirectory(nanopb nanopb)

# SDL2: 仅在 Android 和 iOS 下编译
if(ANDROID OR IOS)
    add_subdirectory(SDL2 SDL2)
endif()

# glfw: 仅在 Windows、macOS 和 非 Android 的 Linux 下编译
# APPLE 包含 macOS 和 iOS，需要排除 iOS
if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    add_subdirectory(glfw glfw)
endif()

add_subdirectory(ktx ktx)
add_subdirectory(zlib zlib)
add_subdirectory(miniz miniz)
add_subdirectory(pvrtc pvrtc)
add_subdirectory(mimalloc mimalloc)
add_subdirectory(yaml-cpp yaml-cpp)
add_subdirectory(tracy)
add_subdirectory(metis metis)
