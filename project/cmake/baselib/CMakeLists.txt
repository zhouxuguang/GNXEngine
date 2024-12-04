
set(BASELIB_SOURCES )
set(baselib_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/BaseLib)
set(baselib_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/BaseLib)
file(GLOB_RECURSE BASELIB_SOURCES CONFIGURE_DEPENDS ${baselib_dir}/*.cpp ${baselib_dir}/*.c ${baselib_dir}/*.h)
file(GLOB_RECURSE BASELIB_INCLUDES CONFIGURE_DEPENDS ${baselib_include_dir}/*.h)

#find_package(ZLIB REQUIRED)

set(ZLIB_SRCS
${baselib_dir}/zlib/adler32.c
${baselib_dir}/zlib/compress.c
${baselib_dir}/zlib/crc32.c
${baselib_dir}/zlib/deflate.c
${baselib_dir}/zlib/gzclose.c
${baselib_dir}/zlib/gzlib.c
${baselib_dir}/zlib/gzread.c
${baselib_dir}/zlib/gzwrite.c
${baselib_dir}/zlib/inflate.c
${baselib_dir}/zlib/infback.c
${baselib_dir}/zlib/inftrees.c
${baselib_dir}/zlib/inffast.c
${baselib_dir}/zlib/trees.c
${baselib_dir}/zlib/uncompr.c
${baselib_dir}/zlib/zutil.c
)

add_library(baselib OBJECT ${BASELIB_SOURCES} ${BASELIB_INCLUDES} ${ZLIB_SRCS})
target_include_directories(baselib PUBLIC ${baselib_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)

