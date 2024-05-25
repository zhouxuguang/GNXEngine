
set(imagecodec_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/ImageCodec)
set(imagecodec_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/ImageCodec)

if(APPLE)  #苹果平台
    file(GLOB IMAGECODEC_SOURCES CONFIGURE_DEPENDS ${imagecodec_dir}/*.cpp 
                                                    ${imagecodec_dir}/*.c 
                                                    ${imagecodec_dir}/*.mm
                                                    ${imagecodec_dir}/*.h
                                                    ${imagecodec_dir}/libtga/*.cpp
                                                    ${imagecodec_dir}/libtga/*.h
                                                    ${imagecodec_dir}/libbmp/*.cpp
                                                    ${imagecodec_dir}/libbmp/*.h) 
else() #其它平台
    file(GLOB_RECURSE IMAGECODEC_SOURCES CONFIGURE_DEPENDS ${imagecodec_dir}/*.cpp ${imagecodec_dir}/*.c ${imagecodec_dir}/*.h) 
endif()

file(GLOB_RECURSE WEBP_SOURCES CONFIGURE_DEPENDS ${imagecodec_dir}/libwebp/*.h 
                                         ${imagecodec_dir}/libwebp/*.c) 

file(GLOB IMAGECODEC_INCLUDES CONFIGURE_DEPENDS ${imagecodec_include_dir}/*.h)

add_library(imagecodec OBJECT ${IMAGECODEC_SOURCES} ${IMAGECODEC_INCLUDES} ${WEBP_SOURCES})

if(APPLE)
    target_link_libraries(imagecodec PRIVATE "-framework Accelerate")
endif()

target_include_directories(imagecodec PUBLIC ${imagecodec_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_include_directories(imagecodec PRIVATE ${imagecodec_dir}/libwebp)
target_link_libraries(imagecodec PRIVATE baselib)
target_compile_options(imagecodec PRIVATE "-fobjc-arc")



