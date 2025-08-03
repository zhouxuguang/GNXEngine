
set(BASELIB_SOURCES )
set(baselib_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/BaseLib)
set(baselib_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/BaseLib)
file(GLOB_RECURSE BASELIB_SOURCES CONFIGURE_DEPENDS ${baselib_dir}/*.cpp ${baselib_dir}/*.c ${baselib_dir}/*.h)
#list(FILTER BASELIB_SOURCES EXCLUDE REGEX "/zlib/")
file(GLOB_RECURSE BASELIB_INCLUDES CONFIGURE_DEPENDS ${baselib_include_dir}/*.h)

set(SOURCE_FILES ${BASELIB_SOURCES} ${BASELIB_INCLUDES})

add_library(baselib STATIC ${SOURCE_FILES})

source_group(TREE ${baselib_dir} FILES ${BASELIB_SOURCES})
source_group(TREE ${baselib_include_dir} FILES ${BASELIB_INCLUDES})

target_include_directories(baselib PUBLIC ${baselib_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
if(WIN32)
    target_link_libraries(baselib PUBLIC zlib)
endif()

