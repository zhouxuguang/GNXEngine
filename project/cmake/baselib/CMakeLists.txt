
set(BASELIB_SOURCES )
set(baselib_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/BaseLib)
set(baselib_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/BaseLib)
file(GLOB_RECURSE BASELIB_SOURCES CONFIGURE_DEPENDS ${baselib_dir}/*.cpp ${baselib_dir}/*.c ${baselib_dir}/*.h)
file(GLOB_RECURSE BASELIB_INCLUDES CONFIGURE_DEPENDS ${baselib_include_dir}/*.h)

find_package(ZLIB REQUIRED)

add_library(baselib OBJECT ${BASELIB_SOURCES} ${BASELIB_INCLUDES})
target_include_directories(baselib PUBLIC ${baselib_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_link_libraries(baselib ${ZLIB_LIBRARIES})



