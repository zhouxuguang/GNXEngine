
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
    file(GLOB IMAGECODEC_SOURCES CONFIGURE_DEPENDS 
                                    ${imagecodec_dir}/*.cpp 
                                    ${imagecodec_dir}/*.c 
                                    ${imagecodec_dir}/*.h
                                    ${imagecodec_dir}/libtga/*.cpp
                                    ${imagecodec_dir}/libtga/*.h
                                    ${imagecodec_dir}/libbmp/*.cpp
                                    ${imagecodec_dir}/libbmp/*.h
                                    ${imagecodec_dir}/libpng/*.c
                                    ${imagecodec_dir}/libpng/*.h) 
endif()

file(GLOB_RECURSE WEBP_SOURCES CONFIGURE_DEPENDS ${imagecodec_dir}/libwebp/*.h 
                                         ${imagecodec_dir}/libwebp/*.c) 

file(GLOB IMAGECODEC_INCLUDES CONFIGURE_DEPENDS ${imagecodec_include_dir}/*.h)

set(JPEG_FILES ${imagecodec_dir}/libjpeg/jcapimin.c
${imagecodec_dir}/libjpeg/jcapistd.c
${imagecodec_dir}/libjpeg/jccoefct.c
${imagecodec_dir}/libjpeg/jccolor.c
${imagecodec_dir}/libjpeg/jcdctmgr.c
${imagecodec_dir}/libjpeg/jchuff.c
${imagecodec_dir}/libjpeg/jcinit.c
${imagecodec_dir}/libjpeg/jcmainct.c
${imagecodec_dir}/libjpeg/jcmarker.c
${imagecodec_dir}/libjpeg/jcmaster.c
${imagecodec_dir}/libjpeg/jcomapi.c
${imagecodec_dir}/libjpeg/jcparam.c
${imagecodec_dir}/libjpeg/jcphuff.c
${imagecodec_dir}/libjpeg/jcprepct.c
${imagecodec_dir}/libjpeg/jcsample.c
${imagecodec_dir}/libjpeg/jctrans.c
${imagecodec_dir}/libjpeg/jdapimin.c
${imagecodec_dir}/libjpeg/jdapistd.c
${imagecodec_dir}/libjpeg/jdatadst.c
${imagecodec_dir}/libjpeg/jdatasrc.c
${imagecodec_dir}/libjpeg/jdcoefct.c
${imagecodec_dir}/libjpeg/jdcolor.c
${imagecodec_dir}/libjpeg/jddctmgr.c
${imagecodec_dir}/libjpeg/jdhuff.c
${imagecodec_dir}/libjpeg/jdinput.c
${imagecodec_dir}/libjpeg/jdmainct.c
${imagecodec_dir}/libjpeg/jdmarker.c
${imagecodec_dir}/libjpeg/jdmaster.c
${imagecodec_dir}/libjpeg/jdmerge.c
${imagecodec_dir}/libjpeg/jdphuff.c
${imagecodec_dir}/libjpeg/jdpostct.c
${imagecodec_dir}/libjpeg/jdsample.c
${imagecodec_dir}/libjpeg/jdtrans.c
${imagecodec_dir}/libjpeg/jerror.c
${imagecodec_dir}/libjpeg/jfdctflt.c
${imagecodec_dir}/libjpeg/jfdctfst.c
${imagecodec_dir}/libjpeg/jfdctint.c
${imagecodec_dir}/libjpeg/jidctflt.c
${imagecodec_dir}/libjpeg/jidctfst.c
${imagecodec_dir}/libjpeg/jidctint.c
${imagecodec_dir}/libjpeg/jidctintelsse.c
${imagecodec_dir}/libjpeg/jidctred.c
${imagecodec_dir}/libjpeg/jmemansi.c
${imagecodec_dir}/libjpeg/jmemmgr.c
${imagecodec_dir}/libjpeg/jutils.c
${imagecodec_dir}/libjpeg/jquant1.c
${imagecodec_dir}/libjpeg/jquant2.c
)

list(APPEND JPEG_FILES ${imagecodec_dir}/libjpeg/jsimd_neon.c ${imagecodec_dir}/libjpeg/jsimd_arm64_neon.S)

# if(arm64-v8a EQUAL {CMAKE_ANDROID_ARCH_ABI})
#     list(APPEND JPEG_FILES ${imagecodec_dir}/libjpeg/jsimd_neon.c ${imagecodec_dir}/libjpeg/jsimd_arm64_neon.S)
# elseif(armeabi-v7a EQUAL {CMAKE_ANDROID_ARCH_ABI})
#     list(APPEND JPEG_FILES ${imagecodec_dir}/libjpeg/jsimd_neon.c ${imagecodec_dir}/libjpeg/jsimd_arm_neon.S)
# endif()


# ${imagecodec_dir}/libjpeg/jsimd_arm_neon.S
# ${imagecodec_dir}/libjpeg/jsimd_arm64_neon.S
# ${imagecodec_dir}/libjpeg/mips_idct_le.S
# ${imagecodec_dir}/libjpeg/jsimd_neon.c

# 额外的png文件
set(PNG_FILES ${imagecodec_dir}/libpng/arm/arm_init.c
${imagecodec_dir}/libpng/arm/filter_neon_intrinsics.c
${imagecodec_dir}/libpng/arm/filter_neon.S
${imagecodec_dir}/libpng/arm/palette_neon_intrinsics.c
${imagecodec_dir}/libpng/contrib/arm-neon/android-ndk.c)

if(ANDROID)
    set(SOURCE_FILES ${IMAGECODEC_SOURCES} 
    ${IMAGECODEC_INCLUDES} 
    ${WEBP_SOURCES} 
    ${JPEG_FILES}
    ${PNG_FILES})
else()
    set(SOURCE_FILES ${IMAGECODEC_SOURCES} ${IMAGECODEC_INCLUDES} ${WEBP_SOURCES})
endif()

add_library(imagecodec OBJECT ${SOURCE_FILES})

if(APPLE)
    target_link_libraries(imagecodec PRIVATE "-framework Accelerate")
endif()

target_include_directories(imagecodec PUBLIC ${imagecodec_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_include_directories(imagecodec PRIVATE ${imagecodec_dir}/libwebp ${imagecodec_dir}/libjpeg)
# if(ANDROID)
#     target_include_directories(imagecodec PRIVATE ${ANDROID_NDK}/sources/android/cpufeatures)
# endif()
target_link_libraries(imagecodec PRIVATE baselib mathutil)
target_compile_options(imagecodec PRIVATE "-fobjc-arc")



