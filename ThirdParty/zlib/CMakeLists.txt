set(ZLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ZLIB_SRCS
${ZLIB_DIR}/adler32.c
${ZLIB_DIR}/compress.c
${ZLIB_DIR}/crc32.c
${ZLIB_DIR}/deflate.c
${ZLIB_DIR}/gzclose.c
${ZLIB_DIR}/gzlib.c
${ZLIB_DIR}/gzread.c
${ZLIB_DIR}/gzwrite.c
${ZLIB_DIR}/inflate.c
${ZLIB_DIR}/infback.c
${ZLIB_DIR}/inftrees.c
${ZLIB_DIR}/inffast.c
${ZLIB_DIR}/trees.c
${ZLIB_DIR}/uncompr.c
${ZLIB_DIR}/zutil.c
)

if(WIN32)
    add_library(zlib STATIC ${ZLIB_SRCS})
    target_include_directories(zlib SYSTEM PUBLIC ${ZLIB_DIR})

    set_property(TARGET zlib PROPERTY FOLDER "ThirdParty")
endif()