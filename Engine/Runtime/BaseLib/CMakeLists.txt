
set(BASELIB_SOURCES )
set(baselib_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(baselib_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE BASELIB_SOURCES CONFIGURE_DEPENDS ${baselib_dir}/*.cpp ${baselib_dir}/*.c ${baselib_dir}/*.h)
file(GLOB_RECURSE BASELIB_INCLUDES CONFIGURE_DEPENDS ${baselib_include_dir}/*.h)

set(SOURCE_FILES ${BASELIB_SOURCES} ${BASELIB_INCLUDES})

if(LINUX)
    find_package(PkgConfig)
    pkg_check_modules(UUID REQUIRED uuid)
endif(LINUX)

add_library(BaseLib OBJECT ${SOURCE_FILES})

source_group(TREE ${baselib_dir} PREFIX source FILES ${BASELIB_SOURCES})
source_group(TREE ${baselib_include_dir} PREFIX include FILES ${BASELIB_INCLUDES})

target_include_directories(BaseLib PRIVATE ${baselib_include_dir})

# DataCompress 使用 zlib 的 compress/inflate/compressBound，所有平台都需要。
# 此前只在 WIN32 下链接，macOS/Linux 上仅靠其它依赖（assimp/ktx 等）间接带入 libz，
# 导致「直接链接 BaseLib 的目标」（如 unittest 的 test_baselib / test_mathutil）链接失败：
#   Undefined symbols: _compress, _compressBound, _inflate, _inflateEnd, _inflateInit2_
# 说明：ThirdParty/zlib 只在 WIN32 下创建 `zlib` 目标，macOS/Linux 使用系统 zlib。
if(WIN32)
    target_link_libraries(BaseLib PUBLIC zlib)
else()
    target_link_libraries(BaseLib PUBLIC z)
endif()

if(LINUX)
    target_link_libraries(BaseLib PRIVATE ${UUID_LIBRARIES})
endif()

target_link_libraries(BaseLib PUBLIC miniz)

set_property(TARGET BaseLib PROPERTY FOLDER "Runtime")
