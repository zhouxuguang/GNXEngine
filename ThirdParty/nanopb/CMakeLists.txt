
add_library(nanopb STATIC
            pb.h
            pb_common.h
            pb_common.c
            pb_encode.h
            pb_encode.c
            pb_decode.h
            pb_decode.c)

target_include_directories(nanopb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET nanopb PROPERTY FOLDER "ThirdParty")